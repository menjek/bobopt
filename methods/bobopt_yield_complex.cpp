#include <methods/bobopt_yield_complex.hpp>

#include <bobopt_config.hpp>
#include <bobopt_debug.hpp>
#include <bobopt_inline.hpp>
#include <bobopt_macros.hpp>
#include <bobopt_optimizer.hpp>
#include <bobopt_text_utils.hpp>
#include <bobopt_utils.hpp>
#include <clang/bobopt_clang_utils.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/Analysis/CFG.h"
#include "clang/Frontend/CompilerInstance.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <algorithm>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_type_traits;

namespace bobopt
{

    namespace methods
    {

        // Configuration.
        //======================================================================

        /// \brief Configuration group name.
        static config_group config("yield complex");
        /// \brief Complexity of not inlined non trivial call. Call defined probably in different TU.
        static config_variable<unsigned> config_call_default_complexity(config, "call_default_complexity", 25u);
        /// \brief Complexity of trivial call. Trivial function is function that doesn't require code generation, i.e., body is empty.
        static config_variable<unsigned> config_call_trivial_complexity(config, "call_trivial_complexity", 1u);
        /// \brief Complexity of function defined as inline. User should be carefull with inline keyword.
        static config_variable<unsigned> config_call_inline_complexity(config, "call_inline_complexity", 5u);
        /// \brief Complexity of contexpr function call. Call is resolved at compile time.
        static config_variable<unsigned> config_call_constexpr_complexity(config, "call_constexpr_complexity", 1u);

        /// \brief Multiplier for body complexity of for loop.
        static config_variable<unsigned> config_multiplier_for(config, "multiplier_for", 20u);
        /// \brief Multiplier for body complexity of while and do/while loops.
        static config_variable<unsigned> config_multiplier_while(config, "multiplier_while", 25u);

        /// \brief Optimal complexity for box execution.
        /// It is equivalent of 2 inner for loops with 5 calls to not inlined non trivial function (20*20*5*25 = 50000).
        static config_variable<unsigned> config_threshold(config, "threshold", 20000u);

        // TU helpers.
        //======================================================================

        namespace
        {

            /// \brief Function detects whether call expression is Bobox yield().
            bool is_yield_call(const CallExpr* call_expr)
            {
                BOBOPT_ASSERT(call_expr != nullptr);

                const CXXMemberCallExpr* member_call_expr = llvm::dyn_cast<CXXMemberCallExpr>(call_expr);
                if (member_call_expr == nullptr)
                {
                    return false;
                }

                const CXXMethodDecl* method_decl = member_call_expr->getMethodDecl();
                const CXXRecordDecl* record_decl = member_call_expr->getRecordDecl();

                return (method_decl->getNameAsString() == "yield") && (record_decl->getNameAsString() == "basic_box");
            }

            /// \brief Function returns complexity of call expression.
            unsigned get_call_complexity(const CallExpr* call_expr)
            {
                BOBOPT_ASSERT(call_expr != nullptr);

                const FunctionDecl* callee = call_expr->getDirectCallee();

                if (callee->hasTrivialBody())
                {
                    return config_call_trivial_complexity.get();
                }

                if (callee->isConstexpr())
                {
                    return config_call_constexpr_complexity.get();
                }

                if (callee->isInlined())
                {
                    return config_call_inline_complexity.get();
                }

                return config_call_default_complexity.get();
            }

            /// \brief Function returns complexity of single CFG element.
            unsigned get_element_complexity(const CFGElement& element)
            {
                if (element.getKind() != CFGElement::Kind::Statement)
                {
                    return 1u;
                }

                const Stmt* stmt = element.castAs<CFGStmt>().getStmt();
                BOBOPT_ASSERT(stmt != nullptr);

                nodes_collector<CallExpr> collector;
                collector.TraverseStmt(const_cast<Stmt*>(stmt));

                unsigned result = 1u;
                for (auto it = collector.nodes_begin(), end = collector.nodes_end(); it != end; ++it)
                {
                    const CallExpr* call_expr = *it;

                    if (is_yield_call(call_expr))
                    {
                        return 0u;
                    }

                    result += get_call_complexity(call_expr);
                }

                return result;
            }

        } // namespace

        // cfg_data implementation.
        //======================================================================

        /// \brief Structure used to hold additional data to analyzer CFG.
        ///
        /// Such additional data are paths passing through block, their
        /// complexities, whether block is yield and some temporary data for
        /// handling loops.
        class cfg_data
        {
        public:
            /// \brief Additional data for single block.
            struct block_data_type
            {
                /// \brief Additional data for paths with the same complexity.
                struct path_data_type
                {
                    std::vector<unsigned> ids;
                    unsigned complexity;
                };

                /// \brief Determines yield state of block.
                enum class yield_state
                {
                    no,
                    planned,
                    present
                };

                yield_state yield;
                std::vector<path_data_type> paths;
                std::unordered_map<unsigned, unsigned> loops;
            };

            typedef std::unordered_map<unsigned, block_data_type> data_type;

        private:
            typedef std::vector<std::pair<unsigned, block_data_type::yield_state> > yields_type;

            static block_data_type::path_data_type make_path_data(unsigned id, unsigned complexity)
            {
                block_data_type::path_data_type result;
                result.ids.push_back(id);
                result.complexity = complexity;
                return result;
            }

        public:

            explicit cfg_data(const CFG& cfg)
                : cfg_(cfg)
                , data_()
            {
                cfg_data_builder builder(cfg);
                data_ = builder.build();
            }

            bool optimize()
            {
                float goodness = get_goodness(data_);
                bool optimized = false;

                for (;;)
                {
                    auto result = optimize_step(data_);
                    if (!result.second)
                    {
                        break;
                    }

                    float temp_goodness = get_goodness(result.first);
                    if (temp_goodness < goodness)
                    {
                        optimized = true;
                        goodness = temp_goodness;
                        data_.swap(result.first);
                    }
                    else
                    {
                        break;
                    }
                }

                return optimized;
            }

            /// \brief Return calculated data.
            data_type get_data() const
            {
                return data_;
            }

        private:
            BOBOPT_NONCOPYMOVABLE(cfg_data);

            /// \brief Builder of additional CFG data from analyzer CFG.
            ///
            /// Class uses context when building additional data and tries to
            /// encapsulate this context into single class for being less
            /// error-prone.
            class cfg_data_builder
            {
            public:
                explicit cfg_data_builder(const CFG& cfg)
                    : cfg_(cfg)
                    , data_()
                    , id_(0)
                    , yields_()
                    , path_stack_()
                    , loop_stack_()
                {
                }

                data_type build(const yields_type& yields = yields_type())
                {
                    yields_ = yields;

                    preprocess();
                    process(cfg_.getEntry(), next_id(), 0u);
                    postprocess();

#ifndef NDEBUG
                    debug_check();
#endif // NDEBUG
                    return data_;
                }

            private:
                BOBOPT_NONCOPYMOVABLE(cfg_data_builder);

                /// \brief Guard internal stacks. Class is responsible for push
                /// and pop of elements using RAII.
                template <typename T>
                class stack_guard_type
                {
                public:
                    stack_guard_type(std::vector<T>& stack, T value)
                        : stack_(stack)
#ifndef NDEBUG
                        , value_(value)
#endif // NDEBUG
                    {
                        BOBOPT_ASSERT(std::find(std::begin(stack_), std::end(stack_), value) == std::end(stack_));
                        stack_.push_back(std::move(value));
                    }

                    ~stack_guard_type()
                    {
                        BOBOPT_ASSERT(!stack_.empty() && (stack_.back() == value_));
                        stack_.pop_back();
                    }

                private:
                    stack_guard_type();
                    BOBOPT_NONCOPYMOVABLE(stack_guard_type);

                    std::vector<T>& stack_;
#ifndef NDEBUG
                    unsigned value_;
#endif // NDEBUG
                };

#ifndef NDEBUG
                void debug_check() const
                {
                    BOBOPT_ASSERT(path_stack_.empty());
                    BOBOPT_ASSERT(loop_stack_.empty());

                    for (const auto& block_pair : data_)
                    {
                        for (const auto& path_data : block_pair.second.paths)
                        {
                            BOBOPT_ASSERT(std::is_sorted(std::begin(path_data.ids), std::end(path_data.ids)));
                            BOBOPT_ASSERT(!path_data.ids.empty());
                            BOBOPT_ASSERT(path_data.ids.back() < id_);
                            BOBOPT_ASSERT(std::adjacent_find(std::begin(path_data.ids), std::end(path_data.ids)) == std::end(path_data.ids));
                        }

                        BOBOPT_ASSERT(block_pair.second.loops.empty());
                    }
                }
#endif // NDEBUG

                BOBOPT_INLINE unsigned next_id()
                {
                    return id_++;
                }

                BOBOPT_INLINE block_data_type::yield_state get_block_yield(unsigned id) const
                {
                    for (const auto& block : yields_)
                    {
                        if (block.first == id)
                        {
                            return block.second;
                        }
                    }

                    return block_data_type::yield_state::no;
                }

                BOBOPT_INLINE bool check_path_stack(unsigned id) const
                {
                    const auto found = std::find(std::begin(path_stack_), std::end(path_stack_), id) != std::end(path_stack_);
                    return !found;
                }

                void preprocess()
                {
                    data_.clear();
                    id_ = 0;
                    path_stack_.clear();
                    loop_stack_.clear();
                }

                void postprocess()
                {
                    for (auto& block_pair : data_)
                    {
                        for (auto& path : block_pair.second.paths)
                        {
                            std::sort(std::begin(path.ids), std::end(path.ids));
                        }

                        block_pair.second.loops.clear();
                    }
                }

                std::vector<unsigned> process(const CFGBlock& block, unsigned path, unsigned complexity)
                {
                    auto block_id = block.getBlockID();

                    // Check whether we are in loop.
                    if (!check_path_stack(block_id))
                    {
                        BOBOPT_ASSERT(!loop_stack_.empty());
                        data_[loop_stack_.back()].loops[path] = complexity;
                        return std::vector<unsigned>();
                    }

                    stack_guard_type<unsigned> guard(path_stack_, block_id);
                    BOBOPT_UNUSED_EXPRESSION(guard);

                    auto& block_data = data_[block_id];
                    block_data.yield = get_block_yield(block_id);

                    unsigned block_complexity = 0u;
                    if (block_data.yield == block_data_type::yield_state::no)
                    {
                        for (const CFGElement& element : block)
                        {
                            auto stmt_comlexity = get_element_complexity(element);
                            block_complexity += stmt_comlexity;

                            // There was call to Bobox yield() in element.
                            // I consider this blocks complexity equal to zero.
                            if (stmt_comlexity == 0u)
                            {
                                block_complexity = 0u;
                                block_data.yield = block_data_type::yield_state::present;
                                break;
                            }
                        }
                    }

                    if (block_data.yield != block_data_type::yield_state::no)
                    {
                        // Save ending path.
                        block_data.paths.push_back(make_path_data(path, complexity));

                        // Start new path and ignore returned paths.
                        process_succ(block, next_id(), 0u);
                        return std::vector<unsigned>();
                    }

                    // Continue path.
                    auto total_complexity = complexity + block_complexity;
                    auto input_path = make_path_data(path, total_complexity);
                    auto from_input_path = process_succ(block, path, total_complexity);
                    append(input_path.ids, from_input_path);
                    block_data.paths.push_back(std::move(input_path));
                    return from_input_path;
                }

                std::vector<unsigned> process_succ(const CFGBlock& block, unsigned path, unsigned complexity)
                {
                    // Branch for loops CFG branching.
                    CFGTerminator terminator = block.getTerminator();
                    if (terminator)
                    {
                        const Stmt* stmt = terminator.getStmt();
                        BOBOPT_ASSERT(stmt != nullptr);

                        if (llvm::dyn_cast<ForStmt>(stmt) != nullptr)
                        {
                            return process_succ_loop(block, path, complexity, config_multiplier_for.get());
                        }

                        if (llvm::dyn_cast<WhileStmt>(stmt) != nullptr)
                        {
                            return process_succ_loop(block, path, complexity, config_multiplier_while.get());
                        }

                        if (llvm::dyn_cast<DoStmt>(stmt) != nullptr)
                        {
                            return process_succ_loop(block, path, complexity, config_multiplier_while.get());
                        }
                    }

                    std::vector<unsigned> return_paths;

                    // Process all successing blocks.
                    // The first branch is deep branch where current path continues.
                    // For all other branches, there's new path created.
                    auto block_it = block.succ_begin();
                    if (block_it != block.succ_end())
                    {
                        append(return_paths, process(**block_it, path, complexity));

                        ++block_it;
                        for (auto end = block.succ_end(); block_it != end; ++block_it)
                        {
                            auto id = next_id();
                            return_paths.push_back(id);
                            append(return_paths, process(**block_it, id, complexity));
                        }
                    }

                    return return_paths;
                }

                std::vector<unsigned> process_succ_loop(const CFGBlock& block, unsigned path, unsigned complexity, unsigned multiplier)
                {
                    auto it = block.succ_begin();
                    BOBOPT_ASSERT(it != block.succ_end());

                    const CFGBlock& body = **it;

                    ++it;
                    BOBOPT_ASSERT(it != block.succ_end());
                    const CFGBlock& skip = **it;

                    ++it;
                    BOBOPT_ASSERT(it == block.succ_end());

                    // Process body with new path using zero complexity.
                    // Complexity value will be multiplied further in evaluation.
                    {
                        stack_guard_type<unsigned> guard(loop_stack_, block.getBlockID());
                        BOBOPT_UNUSED_EXPRESSION(guard);

                        process(body, next_id(), 0u);
                    }

                    std::vector<unsigned> return_paths;

                    auto& block_data = data_[block.getBlockID()];
                    for (auto body_path : block_data.loops)
                    {
                        body_path.second *= multiplier;
                        body_path.second += complexity;

                        return_paths.push_back(body_path.first);
                        append(return_paths, process(skip, body_path.first, body_path.second));
                    }

                    // Process input path as the one that skipped loop body.
                    block_data.loops.clear();
                    append(return_paths, process(skip, path, complexity));
                    return return_paths;
                }

                const CFG& cfg_;

                data_type data_;
                unsigned id_;
                yields_type yields_;
                std::vector<unsigned> path_stack_;
                std::vector<unsigned> loop_stack_;
            }; // cfg_data_builder

            std::pair<data_type, bool> optimize_step(const data_type& src_data)
            {
                yields_type yields;

                // Find all blocks where paths end, i.e., exit and yield blocks.
                std::vector<const block_data_type*> end_blocks;
                for (const auto& block : src_data)
                {
                    if (block.second.yield != block_data_type::yield_state::no)
                    {
                        end_blocks.push_back(&(block.second));
                        yields.emplace_back(block.first, block.second.yield);
                    }
                }

                auto exit_block_it = src_data.find(cfg_.getExit().getBlockID());
                BOBOPT_ASSERT(exit_block_it != std::end(src_data));
                end_blocks.push_back(&(exit_block_it->second));

                // Iterate through all blocks and calculate what we can achieve
                // by placing yield inside block.
                float goodness = std::numeric_limits<float>::max();
                unsigned block_id = 0u;
                bool optimized = false;
                for (const auto& block : src_data)
                {
                    if (block.second.yield != block_data_type::yield_state::no)
                    {
                        continue;
                    }

                    if (block.first == cfg_.getExit().getBlockID())
                    {
                        continue;
                    }

                    auto result = optimize_block(block.second, end_blocks);
                    if (result.second && (result.first < goodness))
                    {
                        block_id = block.first;
                        goodness = result.first;
                        optimized = true;
                    }
                }

                // If there is block worth optimizing, insert yield into
                // destination data structure and recalculate cfg.
                if (optimized)
                {
                    yields.emplace_back(block_id, block_data_type::yield_state::planned);

                    cfg_data_builder builder(cfg_);
                    return std::make_pair(builder.build(yields), true);
                }

                return std::make_pair(data_type(), false);
            }

            static std::vector<block_data_type::path_data_type>::const_iterator find_path(unsigned id, const block_data_type& block)
            {
                return std::find_if(std::begin(block.paths), std::end(block.paths), [id](const block_data_type::path_data_type & path) {
                    return std::find(std::begin(path.ids), std::end(path.ids), id) != std::end(path.ids);
                });
            }

            std::pair<float, bool> optimize_block(const block_data_type& block, const std::vector<const block_data_type*>& end_blocks)
            {
                unsigned distance = 0u;
                unsigned count = 0u;

                // Check whether block is worth optimizing.
                bool over_threshold = false;
                for (const auto& path : block.paths)
                {
                    unsigned ids_size = static_cast<unsigned>(path.ids.size());
                    count += ids_size;

                    if (path.complexity > config_threshold.get())
                    {
                        over_threshold = true;
                    }

                    distance += ids_size * value_distance(config_threshold.get(), path.complexity);
                }

                if (!over_threshold)
                {
                    return std::make_pair(0.0f, false);
                }

                // Evaluate blocks at the end of paths.
                for (const auto* end_block : end_blocks)
                {
                    for (const auto& path : end_block->paths)
                    {
                        const auto path_distance = value_distance(config_threshold.get(), path.complexity);
                        for (auto id : path.ids)
                        {
                            ++count;

                            const auto found_it = find_path(id, block);
                            if (found_it == std::end(block.paths))
                            {
                                distance += path_distance;
                                continue;
                            }

                            auto new_complexity = path.complexity - found_it->complexity;
                            distance += value_distance(config_threshold.get(), new_complexity);
                        }
                    }
                }

                return std::make_pair(distance / static_cast<float>(count), true);
            }

            float get_goodness(const data_type& data) const
            {
                auto exit_it = data.find(cfg_.getExit().getBlockID());
                BOBOPT_ASSERT(exit_it != std::end(data));
                BOBOPT_ASSERT(exit_it->second.yield != block_data_type::yield_state::planned);

                unsigned distance = 0u;
                unsigned count = 0u;

                // Paths ending in EXIT block.
                for (const auto& path : exit_it->second.paths)
                {
                    const auto ids_count = static_cast<unsigned>(path.ids.size());
                    count += ids_count;
                    distance += ids_count * value_distance(config_threshold.get(), path.complexity);
                }

                // Paths ending in yielded blocks.
                for (const auto& block : data)
                {
                    if (block.second.yield != block_data_type::yield_state::no)
                    {
                        for (const auto& path : block.second.paths)
                        {
                            const auto ids_count = static_cast<unsigned>(path.ids.size());
                            count += ids_count;
                            distance += ids_count * value_distance(config_threshold.get(), path.complexity);
                        }
                    }
                }

                return distance / static_cast<float>(count);
            }

            const CFG& cfg_;
            data_type data_;
        };

        // yield_complex implementation.
        //==============================================================================

        // constants:

        const yield_complex::method_override yield_complex::BOX_EXEC_METHOD_OVERRIDES[] = { { { "sync_mach_etwas" }, { "bobox::basic_box" } },
                                                                                            { { "async_mach_etwas" }, { "bobox::basic_box" } },
                                                                                            { { "body_mach_etwas" }, { "bobox::basic_box" } } };

        /// \brief Create default constructed unusable object.
        yield_complex::yield_complex()
            : box_(nullptr)
            , replacements_(nullptr)
        {
        }

        /// \brief Deletable through pointer to base.
        yield_complex::~yield_complex()
        {
        }

        /// \brief Inherited optimization member function.
        /// It just checks and stores optmization parameters and forwards job to dedicated member function.
        void yield_complex::optimize(CXXRecordDecl* box, tooling::Replacements* replacements)
        {
            BOBOPT_ASSERT(box != nullptr);
            BOBOPT_ASSERT(replacements != nullptr);

            box_ = box;
            replacements_ = replacements;

            optimize_methods();
        }

        /// \brief Main optimization pass.
        /// Function iterates through box methods and if it matches method in array it calls dedicated function to
        /// optimize single method.
        void yield_complex::optimize_methods()
        {
            BOBOPT_ASSERT(box_ != nullptr);

            for (auto method_it = box_->method_begin(); method_it != box_->method_end(); ++method_it)
            {
                CXXMethodDecl* method = *method_it;

                for (const auto& exec_method : BOX_EXEC_METHOD_OVERRIDES)
                {
                    if ((method->getNameAsString() == exec_method.method_name) && overrides(method, exec_method.parent_name))
                    {
                        optimize_method(method);
                    }
                }
            }
        }

        /// \brief Main optimization pass for single method.
        void yield_complex::optimize_method(CXXMethodDecl* method)
        {
            BOBOPT_ASSERT(method != nullptr);

            if (!method->hasBody())
            {
                return;
            }

            CompoundStmt* body = llvm::dyn_cast_or_null<CompoundStmt>(method->getBody());
            if (body == nullptr)
            {
                return;
            }

            CFG::BuildOptions options;
            std::unique_ptr<CFG> cfg(CFG::buildCFG(method, body, &method->getASTContext(), options));

            if (cfg == nullptr)
            {
                llvm::errs() << "[ERROR] Failed to build CFG from function body.\n";
                return;
            }

            optimize_body(method, body, *cfg);
        }

        typedef std::unordered_map<unsigned, const CFGBlock*> id_block_map;

        /// \brief Build map from block ids to pointers to blocks.
        static id_block_map build_block_map(const CFG& cfg)
        {
            id_block_map result;

            std::vector<const CFGBlock*> proceed;
            proceed.reserve(cfg.size());
            proceed.push_back(&cfg.getEntry());

            while (!proceed.empty())
            {
                const CFGBlock* block = proceed.back();
                proceed.pop_back();

                BOBOPT_ASSERT(result.count(block->getBlockID()) == 0);
                result[block->getBlockID()] = block;

                for (auto it = block->succ_begin(), end = block->succ_end(); it != end; ++it)
                {
                    const CFGBlock* succ = *it;
                    if (result.count(succ->getBlockID()) == 0)
                    {
                        proceed.push_back(succ);
                    }
                }
            }

            return result;
        }

        /// \brief Emit box optimization header.
        static void emit_header(CXXRecordDecl* decl)
        {
            llvm::raw_ostream& out = llvm::outs();

            out.changeColor(llvm::raw_ostream::WHITE, true);
            out << "[yield complex]";
            out.resetColor();
            out << " optimization of box ";
            out.changeColor(llvm::raw_ostream::MAGENTA, true);
            out << decl->getNameAsString();
            out.resetColor();
            out << "\n\n";
        }

        /// \brief Optimize member function body represented by CFG.
        void yield_complex::optimize_body(CXXMethodDecl* method, CompoundStmt* body, const CFG& cfg)
        {
            cfg_data data(cfg);
            if (!data.optimize())
            {
                return;
            }

            auto optimized_data = data.get_data();
            auto map = build_block_map(cfg);

            std::vector<unsigned> ids;
            for (const auto& block : optimized_data)
            {
                if (block.second.yield == cfg_data::block_data_type::yield_state::planned)
                {
                    ids.push_back(block.first);
                }
            }

            nodes_collector<CompoundStmt> compound_collector;
            compound_collector.TraverseStmt(body);
            std::vector<const CompoundStmt*> stmts(compound_collector.nodes_begin(), compound_collector.nodes_end());

            endl_ = detect_line_end(get_optimizer().get_compiler().getSourceManager(), box_);

            if (get_optimizer().verbose())
            {
                emit_header(box_);

                auto& diag = get_optimizer().get_diagnostic();
                diag.emit(diag.get_message_decl(diagnostic_message::types::info, method, "method takes too long time on some paths:"));
            }

            // Insert yields.
            for (auto id : ids)
            {
                BOBOPT_ASSERT(map.count(id) == 1);
                const CFGBlock& block = *(map.find(id)->second);
                BOBOPT_CHECK(inserter(block, stmts));
            }
        }

        namespace
        {

            /// \brief Helper for looking up statement in AST subtree.
            class recursive_stmt_find_helper : public RecursiveASTVisitor<recursive_stmt_find_helper>
            {
            public:
                recursive_stmt_find_helper(const Stmt* stmt) : stmt_(stmt)
                {
                }

                bool VisitStmt(Stmt* stmt)
                {
                    return !(stmt_ == stmt);
                }

            private:
                const Stmt* stmt_;
            };

        } // namespace

        /// \brief Final phase for inserting \c yield() call to source code.
        void yield_complex::inserter_invoke(Stmt* stmt, SourceLocation location) const
        {
            auto& sm = get_optimizer().get_compiler().getSourceManager();

            bool update_code = false;
            if (get_optimizer().verbose())
            {
                auto& diag = get_optimizer().get_diagnostic();
                diag.emit(diag.get_message_stmt(diagnostic_message::types::suggestion, stmt, "placing yield() call just before statement:"));

                if (get_optimizer().get_mode() == MODE_INTERACTIVE)
                {
                    if (ask_yesno("Do you want to place yield() call to code?"))
                    {
                        update_code = true;
                    }
                    llvm::outs() << "\n\n";
                }
            }

            if (update_code || (get_optimizer().get_mode() == MODE_BUILD))
            {
                std::string yield_code = "yield();" + endl_ + location_indent(sm, location);
                replacements_->insert(Replacement(sm, location, 0, yield_code));
            }
        }

        /// \brief Helper to analyze subtree of single statement in compound statement.
        bool yield_complex::inserter_helper(Stmt* dst_stmt, const Stmt* src_stmt) const
        {
            recursive_stmt_find_helper helper(src_stmt);

            IfStmt* if_stmt = llvm::dyn_cast<IfStmt>(dst_stmt);
            if (if_stmt != nullptr)
            {
                if (!helper.TraverseStmt(if_stmt->getCond()))
                {
                    inserter_invoke(if_stmt, if_stmt->getLocStart());
                    return true;
                }
                return false;
            }

            ForStmt* for_stmt = llvm::dyn_cast<ForStmt>(dst_stmt);
            if (for_stmt != nullptr)
            {
                if (!helper.TraverseStmt(for_stmt->getInit()))
                {
                    inserter_invoke(for_stmt, for_stmt->getLocStart());
                    return true;
                }

                recursive_stmt_find_helper helper1(src_stmt);
                if (!helper1.TraverseStmt(for_stmt->getInc()))
                {
                    const CompoundStmt* body = llvm::dyn_cast_or_null<const CompoundStmt>(for_stmt->getBody());
                    inserter_invoke(for_stmt->getInc(), body->getRBracLoc());
                    return true;
                }

                return false;
            }

            WhileStmt* while_stmt = llvm::dyn_cast<WhileStmt>(dst_stmt);
            if (while_stmt != nullptr)
            {
                if (!helper.TraverseStmt(while_stmt->getCond()))
                {
                    inserter_invoke(while_stmt, while_stmt->getLocStart());
                    return true;
                }
                return false;
            }

            SwitchStmt* switch_stmt = llvm::dyn_cast<SwitchStmt>(dst_stmt);
            if (switch_stmt != nullptr)
            {
                if (!helper.TraverseStmt(switch_stmt->getCond()))
                {
                    inserter_invoke(switch_stmt, switch_stmt->getLocStart());
                    return true;
                }
                return false;
            }

            CompoundStmt* compound_stmt = llvm::dyn_cast<CompoundStmt>(dst_stmt);
            if (compound_stmt != nullptr)
            {
                return false;
            }

            if (!helper.TraverseStmt(dst_stmt))
            {
                inserter_invoke(dst_stmt, dst_stmt->getLocStart());
                return true;
            }

            return false;
        }

        /// \brief Helper for insert yield for block into compound statement.
        bool yield_complex::inserter(const CFGBlock& block, const CompoundStmt* stmt) const
        {
            if (block.empty())
            {
                return false;
            }

            // Find first statement in block.
            const Stmt* block_stmt = nullptr;
            for (auto it = block.begin(), end = block.end(); it != end; ++it)
            {
                if (it->getKind() == CFGElement::Kind::Statement)
                {
                    block_stmt = it->castAs<CFGStmt>().getStmt();
                    break;
                }
            }

            if (block_stmt == nullptr)
            {
                return false;
            }

            // Find whether this statement is in compound.
            for (auto it = stmt->body_begin(), end = stmt->body_end(); it != end; ++it)
            {
                Stmt* local_stmt = *it;
                if (inserter_helper(local_stmt, block_stmt))
                {
                    return true;
                }
            }

            return false;
        }

        /// \brief Helper for insert of block yield into single compound statement from set of compound statements.
        bool yield_complex::inserter(const CFGBlock& block, const std::vector<const CompoundStmt*>& stmts) const
        {
            for (const auto* stmt : stmts)
            {
                if (inserter(block, stmt))
                {
                    return true;
                }
            }
            return false;
        }

    } // namespace

    basic_method* create_yield_complex()
    {
        return new methods::yield_complex;
    }

} // namespace
