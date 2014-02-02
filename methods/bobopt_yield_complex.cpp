#include <methods/bobopt_yield_complex.hpp>

#include <bobopt_debug.hpp>
#include <bobopt_inline.hpp>
#include <bobopt_macros.hpp>
#include <bobopt_utils.hpp>
#include <clang/bobopt_clang_utils.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/Analysis/CFG.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <algorithm>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace clang;
using namespace clang::ast_type_traits;

namespace bobopt
{

    namespace methods
    {

        // Algorithm constants.
        //======================================================================

        /// \brief Complexity of not inlined non trivial call. Call defined probably in different TU.
        static const unsigned call_default_complexity = 25u;
        /// \brief Complexity of trivial call. Trivial function is function that doesn't require code generation, i.e., body is empty.
        static const unsigned call_trivial_complexity = 1u;
        /// \brief Complexity of function defined as inline. User should be carefull with inline keyword.
        static const unsigned call_inline_complexity = 5u;
        /// \brief Complexity of contexpr function call. Call is resolved at compile time.
        static const unsigned call_constexpr_complexity = 1u;

        /// \brief Multiplier for body complexity of for loop.
        static const unsigned multiplier_for = 20u;
        /// \brief Multiplier for body complexity of while and do/while loops.
        static const unsigned multiplier_while = 25u;

        /// \brief Optimal complexity for box execution.
        /// It is equivalent of 2 inner for loops with 5 calls to not inlined non trivial function (20*20*5*25 = 50000).
        static const unsigned threshold = 20000u;

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
                    return call_trivial_complexity;
                }

                if (callee->isConstexpr())
                {
                    return call_constexpr_complexity;
                }

                if (callee->isInlined())
                {
                    return call_inline_complexity;
                }

                return call_default_complexity;
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

                ast_nodes_collector<CallExpr> collector;
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

            /// \brief Append \c std::vector to another \c std::vector.
            template <typename T, typename A>
            void append(std::vector<T, A>& dst, const std::vector<T, A>& src)
            {
                dst.insert(std::end(dst), std::begin(src), std::end(src));
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

            /// \brief Additional data for single block.
            struct block_data_type
            {
                /// \brief Additional data for paths with the same complexity.
                struct path_data_type
                {
                    std::vector<unsigned> ids;
                    unsigned complexity;
                };

                bool yield;
                std::vector<path_data_type> paths;
                std::unordered_map<unsigned, unsigned> loops;
            };

            typedef std::unordered_map<unsigned, block_data_type> data_type;

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

            /// \brief Apply data to source code.
            void apply() const
            {
                BOBOPT_TODO("Implement.")
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

                data_type build(const std::vector<unsigned>& yields = std::vector<unsigned>())
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

                BOBOPT_INLINE bool force_yield(unsigned id) const
                {
                    return std::binary_search(std::begin(yields_), std::end(yields_), id);
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

                    std::sort(std::begin(yields_), std::end(yields_));
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
                    block_data.yield = force_yield(block_id);

                    unsigned block_complexity = 0u;
                    if (!block_data.yield)
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
                                block_data.yield = true;
                                break;
                            }
                        }
                    }

                    if (block_data.yield)
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
                            return process_succ_loop(block, path, complexity, multiplier_for);
                        }

                        if (llvm::dyn_cast<WhileStmt>(stmt) != nullptr)
                        {
                            return process_succ_loop(block, path, complexity, multiplier_while);
                        }

                        if (llvm::dyn_cast<DoStmt>(stmt) != nullptr)
                        {
                            return process_succ_loop(block, path, complexity, multiplier_while);
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
                std::vector<unsigned> yields_;
                std::vector<unsigned> path_stack_;
                std::vector<unsigned> loop_stack_;
            }; // cfg_data_builder

            std::pair<data_type, bool> optimize_step(const data_type& src_data)
            {
                std::vector<unsigned> yields;

                // Find all blocks where paths end, i.e., exit and yield blocks.
                std::vector<const block_data_type*> end_blocks;
                for (const auto& block : src_data)
                {
                    if (block.second.yield)
                    {
                        end_blocks.push_back(&(block.second));
                        yields.push_back(block.first);
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
                    if (block.second.yield)
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
                    yields.push_back(block_id);

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

                    if (path.complexity > threshold)
                    {
                        over_threshold = true;
                    }

                    distance += ids_size * value_distance(threshold, path.complexity);
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
                        const auto path_distance = value_distance(threshold, path.complexity);
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
                            distance += value_distance(threshold, new_complexity);
                        }
                    }
                }

                return std::make_pair(distance / static_cast<float>(count), true);
            }

            float get_goodness(const data_type& data) const
            {
                auto exit_it = data.find(cfg_.getExit().getBlockID());
                BOBOPT_ASSERT(exit_it != std::end(data));
                BOBOPT_ASSERT(!(exit_it->second.yield));

                unsigned distance = 0u;
                unsigned count = 0u;

                // Paths ending in EXIT block.
                for (const auto& path : exit_it->second.paths)
                {
                    const auto ids_count = static_cast<unsigned>(path.ids.size());
                    count += ids_count;
                    distance += ids_count * value_distance(threshold, path.complexity);
                }

                // Paths ending in yielded blocks.
                for (const auto& block : data)
                {
                    if (block.second.yield)
                    {
                        for (const auto& path : block.second.paths)
                        {
                            const auto ids_count = static_cast<unsigned>(path.ids.size());
                            count += ids_count;
                            distance += ids_count * value_distance(threshold, path.complexity);
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
            replacements_ = replacements_;

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
        void yield_complex::optimize_method(exec_function_type method)
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

            optimize_body(*cfg);
        }

        /// \brief Optimize member function body represented by CFG.
        void yield_complex::optimize_body(const CFG& cfg)
        {
            cfg_data data(cfg);
            if (data.optimize())
            {
                data.apply();
            }
        }

    } // namespace

    basic_method* create_yield_complex()
    {
        return new methods::yield_complex;
    }

} // namespace
