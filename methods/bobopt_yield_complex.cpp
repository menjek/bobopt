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
#include <memory>
#include <numeric>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;
using namespace clang;
using namespace clang::ast_type_traits;

namespace bobopt
{

    namespace methods
    {

        // helpers implementation.
        //==============================================================================

        static const unsigned call_default_complexity = 50u;
        static const unsigned call_trivial_complexity = 10u;
        static const unsigned call_inline_complexity = 5u;
        static const unsigned call_constexpr_complexity = 1u;

        static const unsigned multiplier_for = 15u;
        static const unsigned multiplier_while = 20u;

        static const unsigned threshold = 100000u;

        template <typename T, typename U>
        static auto value_distance(T a, U b) -> decltype((a > b) ? (a - b) : (b - a))
        {
            return (a > b) ? (a - b) : (b - a);
        }

        static bool is_yield(const CallExpr* call_expr)
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

        static unsigned calc_call_complexity(const CallExpr* call_expr)
        {
            BOBOPT_ASSERT(call_expr != nullptr);

            const FunctionDecl* callee = call_expr->getDirectCallee();

            if (callee->isConstexpr())
            {
                return call_constexpr_complexity;
            }

            if (callee->isInlined())
            {
                return call_inline_complexity;
            }

            if (callee->hasTrivialBody())
            {
                return call_trivial_complexity;
            }

            return call_default_complexity;
        }

        static unsigned calc_element_complexity(const CFGElement& element)
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

                if (is_yield(call_expr))
                {
                    return 0u;
                }

                result += calc_call_complexity(call_expr);
            }

            return result;
        }

        // cfg_data implementation.
        //==============================================================================

        class cfg_data
        {
            struct block_data
            {
                struct path_data
                {
                    std::vector<unsigned> ids;
                    unsigned complexity;
                };

                bool yield;
                std::vector<path_data> paths;
                std::unordered_map<unsigned, unsigned> temps;
            };

            typedef std::unordered_map<unsigned, block_data> data_type;

        public:
            explicit cfg_data(const CFG& cfg) : cfg_(cfg)
            {
                cfg_builder builder;
                builder.process(data_, cfg.getEntry());

                BOBOPT_ASSERT(cfg_builder::debug_check(cfg, data_));
            }

            void optimize()
            {
                data_type temp(data_);
                sweep_data(temp);
                float goodness = calc_goodness(data_);

                for (;;)
                {
                    if (!optimize_step(temp, data_))
                    {
                        break;
                    }

                    float temp_goodness = calc_goodness(temp);
                    if (temp_goodness < goodness)
                    {
                        goodness = temp_goodness;
                        data_.swap(temp);
                        sweep_data(temp);
                    }
                    else
                    {
                        break;
                    }
                }
            }

            void apply() const
            {
                BOBOPT_TODO("Implement.")
            }

        private:
            BOBOPT_NONCOPYMOVABLE(cfg_data);

            class cfg_builder
            {
            public:
                explicit cfg_builder(bool build = true)
                    : build_(build)
                    , max_id_(0)
                    , stack_()
                {
                }

                void process(data_type& data, const CFGBlock& entry_block)
                {
                    process_cfg_block(data, entry_block, next_id(), 0u);
                    postprocess(data);
                }

#ifndef NDEBUG
                static bool debug_check(const CFG& cfg, data_type& data)
                {
                    // Do/While makes all other checks irrelevant.
                    for (const auto& block : cfg)
                    {
                        CFGTerminator terminator = block->getTerminator();
                        if (terminator)
                        {
                            if (llvm::dyn_cast<DoStmt>(terminator.getStmt()) != nullptr)
                            {
                                return true;
                            }
                        }
                    }

                    // Check uniqueness of path id for every block.
                    for (const auto& block : data)
                    {
                        using namespace std;

                        const block_data& item = block.second;

                        vector<unsigned> ids = accumulate(
                            begin(item.paths), end(item.paths), vector<unsigned>(), [](vector<unsigned> & lhs, const block_data::path_data & path) {
                                lhs.insert(end(lhs), begin(path.ids), end(path.ids));
                                return lhs;
                            });

                        sort(begin(ids), end(ids));
                        if (adjacent_find(begin(ids), end(ids)) != end(ids))
                        {
                            return false;
                        }
                    }

                    return true;
                }
#endif // NDEBUG

            private:
                template <typename T>
                class stack_scope
                {
                public:
                    stack_scope(std::vector<T>& stack, T value)
                        : stack_(stack)
                        , cycle_(false)
                        , value_(value)
                    {
                        if (std::find(std::begin(stack_), std::end(stack_), value_) != std::end(stack_))
                        {
                            cycle_ = true;
                        }

                        stack_.push_back(value_);
                    }

                    ~stack_scope()
                    {
                        BOBOPT_ASSERT(!stack_.empty() && (stack_.back() == value_));
                        stack_.pop_back();
                    }

                    bool cycle() const
                    {
                        return cycle_;
                    }

                private:
                    stack_scope();
                    BOBOPT_NONCOPYMOVABLE(stack_scope);

                    std::vector<T>& stack_;
                    bool cycle_;
                    unsigned value_;
                };

                unsigned next_id()
                {
                    return max_id_++;
                }

                static void postprocess(data_type& data)
                {
                    for (auto& block_pair : data)
                    {
                        for (auto& path : block_pair.second.paths)
                        {
                            std::sort(std::begin(path.ids), std::end(path.ids));
                        }

                        block_pair.second.temps.clear();
                    }
                }

                std::vector<unsigned> process_cfg_block(data_type& data, const CFGBlock& block, unsigned path, unsigned complexity)
                {
                    unsigned block_id = block.getBlockID();
                    block_data& block_item = data[block_id];

                    stack_scope<unsigned> guard(stack_, block_id);
                    if (guard.cycle())
                    {
                        // In loop?
                        CFGTerminator terminator = block.getTerminator();
                        if (terminator)
                        {
                            const Stmt* stmt = terminator.getStmt();
                            BOBOPT_ASSERT(stmt != nullptr);

                            if ((llvm::dyn_cast<ForStmt>(stmt) != nullptr) || (llvm::dyn_cast<WhileStmt>(stmt) != nullptr) ||
                                (llvm::dyn_cast<DoStmt>(stmt) != nullptr))
                            {
                                // Yep.
                                block_item.temps[path] = complexity;
                                return std::vector<unsigned>();
                            }
                        }
                    }

                    if (build_)
                    {
                        block_item.yield = false;
                    }

                    unsigned block_complexity = 0u;
                    for (const CFGElement& element : block)
                    {
                        unsigned stmt_comlexity = calc_element_complexity(element);
                        block_complexity += stmt_comlexity;

                        if (stmt_comlexity == 0u)
                        {
                            BOBOPT_ASSERT(build_ || block_item.yield);
                            block_item.yield = true;
                            break;
                        }

                        if (!build_ && block_item.yield)
                        {
                            break;
                        }
                    }

                    if (block_item.yield)
                    {
                        block_data::path_data input_path;
                        input_path.ids.push_back(path);
                        input_path.complexity = complexity + block_complexity;
                        block_item.paths.push_back(input_path);

                        block_data::path_data new_path;
                        new_path.ids.push_back(next_id());
                        new_path.complexity = 0;
                        process_succ(data, block, new_path);
                        return std::vector<unsigned>();
                    }

                    block_data::path_data input_path;
                    input_path.ids.push_back(path);
                    input_path.complexity = complexity + block_complexity;
                    auto paths = process_succ(data, block, input_path);
                    block_item.paths.push_back(input_path);
                    return paths;
                }

                std::vector<unsigned> process_succ(data_type& data, const CFGBlock& block, block_data::path_data& path)
                {
                    BOBOPT_ASSERT(path.ids.size() == 1);

                    CFGTerminator terminator = block.getTerminator();
                    if (terminator)
                    {
                        const Stmt* stmt = terminator.getStmt();
                        BOBOPT_ASSERT(stmt != nullptr);

                        if (llvm::dyn_cast<ForStmt>(stmt) != nullptr)
                        {
                            return process_succ_loop(data, block, path, multiplier_for);
                        }

                        if (llvm::dyn_cast<WhileStmt>(stmt) != nullptr)
                        {
                            return process_succ_loop(data, block, path, multiplier_while);
                        }

                        if (llvm::dyn_cast<DoStmt>(stmt) != nullptr)
                        {
                            return process_succ_loop(data, block, path, multiplier_while);
                        }
                    }

                    std::vector<unsigned> created_paths;

                    auto block_it = block.succ_begin();
                    if (block_it != block.succ_end())
                    {
                        auto deep_path = process_cfg_block(data, **block_it, path.ids[0], path.complexity);
                        path.ids.insert(std::end(path.ids), std::begin(deep_path), std::end(deep_path));
                        created_paths.insert(std::end(created_paths), std::begin(deep_path), std::end(deep_path));

                        ++block_it;
                        for (auto end = block.succ_end(); block_it != end; ++block_it)
                        {
                            unsigned id = next_id();
                            path.ids.push_back(id);
                            created_paths.push_back(id);
                            auto new_paths = process_cfg_block(data, **block_it, id, path.complexity);
                            path.ids.insert(std::end(path.ids), std::begin(new_paths), std::end(new_paths));
                            created_paths.insert(std::end(created_paths), std::begin(new_paths), std::end(new_paths));
                        }
                    }

                    return created_paths;
                }

                std::vector<unsigned> process_succ_loop(data_type& data, const CFGBlock& block, block_data::path_data& path, unsigned multiplier)
                {
                    BOBOPT_ASSERT(path.ids.size() == 1);
                    BOBOPT_ASSERT(data.count(block.getBlockID()) == 1);

                    auto path_id = path.ids[0];
                    auto path_complexity = path.complexity;
                    //auto block_id = block.getBlockID();
                    //auto& block_item = data[block_id];

                    auto it = block.succ_begin();
                    BOBOPT_ASSERT(it != block.succ_end());

                    const CFGBlock& body = **it;

                    ++it;
                    BOBOPT_ASSERT(it != block.succ_end());
                    const CFGBlock& skip = **it;

                    ++it;
                    BOBOPT_ASSERT(it == block.succ_end());

                    // Process body but with zero complexity
                    // because value will be multiplied later.
                    process_cfg_block(data, body, path_id, path_complexity);

                    std::vector<unsigned> created_paths;

                    block_data& data_item = data[block.getBlockID()];
                    for (auto body_path : data_item.temps)
                    {
                        body_path.second *= multiplier;
                        body_path.second += path.complexity;

                        if (body_path.first == path_id)
                        {
                            path.complexity = body_path.second;

                            auto paths = process_cfg_block(data, skip, body_path.first, body_path.second);
                            path.ids.insert(std::end(path.ids), std::begin(paths), std::end(paths));
                            created_paths.insert(std::end(created_paths), std::begin(paths), std::end(paths));
                        }
                        else
                        {
                            block_data::path_data new_path;
                            new_path.ids.push_back(body_path.first);
                            new_path.complexity = body_path.second;

                            created_paths.push_back(new_path.ids[0]);
                            auto paths = process_cfg_block(data, skip, body_path.first, body_path.second);
                            new_path.ids.insert(std::end(new_path.ids), std::begin(paths), std::end(paths));
                            created_paths.insert(std::end(created_paths), std::begin(paths), std::end(paths));

                            //block_item.paths.push_back(new_path);
                        }
                    }

                    data_item.temps.clear();

                    block_data::path_data new_path;
                    new_path.ids.push_back(next_id());
                    new_path.complexity = path.complexity;

                    created_paths.push_back(new_path.ids[0]);
                    auto paths = process_cfg_block(data, skip, new_path.ids[0], new_path.complexity);
                    new_path.ids.insert(std::end(new_path.ids), std::begin(paths), std::end(paths));
                    created_paths.insert(std::end(created_paths), std::begin(paths), std::end(paths));

                    //block_item.paths.push_back(new_path);

                    return created_paths;
                }

                bool build_;
                unsigned max_id_;
                std::vector<unsigned> stack_;
            };

            static void sweep_data(data_type& data)
            {
                for (auto& block_pair : data)
                {
                    block_pair.second.paths.clear();
                    block_pair.second.temps.clear();
                }
            }

            bool optimize_step(data_type& dst_data, const data_type& src_data)
            {
                BOBOPT_ASSERT((src_data.size() == dst_data.size()) && (src_data.size() == cfg_.size()));

                // Find all blocks where paths end, i.e., exit and yielded blocks.
                std::vector<const block_data*> end_blocks;
                for (const auto& block : src_data)
                {
                    if (block.second.yield)
                    {
                        end_blocks.push_back(&(block.second));
                    }
                }

                auto exit_it = src_data.find(cfg_.getExit().getBlockID());
                BOBOPT_ASSERT(exit_it != std::end(src_data));
                end_blocks.push_back(&(exit_it->second));

                // Iterate through all blocks and calculate what we can achieve
                // by placing yield inside block.
                float min_goodness = std::numeric_limits<float>::max();
                unsigned min_block = 0u;
                for (const auto& block : src_data)
                {
                    if (block.second.yield)
                    {
                        continue;
                    }

                    auto result = optimize_block(block.second, end_blocks);
                    if (result.second && (result.first < min_goodness))
                    {
                        min_block = block.first;
                        min_goodness = result.first;
                    }
                }

                // If there was block worth optimizing, insert yield into destination
                // data structure and recalculate cfg.
                if (min_block != 0u)
                {
                    auto max_block_it = dst_data.find(min_block);
                    BOBOPT_ASSERT(max_block_it != std::end(dst_data));
                    max_block_it->second.yield = true;

                    cfg_builder builder(false);
                    builder.process(dst_data, cfg_.getEntry());

                    return true;
                }

                return false;
            }

            static std::vector<block_data::path_data>::const_iterator find_path(unsigned id, const block_data& block)
            {
                using namespace std;

                return find_if(begin(block.paths), end(block.paths), [id](const block_data::path_data & path) {
                    return find(begin(path.ids), end(path.ids), id) != end(path.ids);
                });
            }

            std::pair<float, bool> optimize_block(const block_data& block, const std::vector<const block_data*>& end_blocks)
            {
                unsigned distance = 0u;
                unsigned count = 0u;
                
                // Check whether block is worth optimizing.
                bool over_threshold = false;
                for (const auto& path : block.paths)
                {
                    unsigned ids_count = static_cast<unsigned>(path.ids.size());
                    count += ids_count;
                    
                    if (path.complexity > threshold)
                    {
                        over_threshold = true;
                    }

                    distance += ids_count * value_distance(threshold, path.complexity);
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

            float calc_goodness(const data_type& data) const
            {
                auto exit_it = data.find(cfg_.getExit().getBlockID());
                BOBOPT_ASSERT(exit_it != std::end(data));
                BOBOPT_ASSERT(!(exit_it->second.yield));

                unsigned distance = 0u;
                unsigned count = 0u;

                // Paths ending in EXIT block.
                for (const auto& path : exit_it->second.paths)
                {
                    BOBOPT_TODO("Extract to function.")
                    auto ids_count = static_cast<unsigned>(path.ids.size());

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
                            BOBOPT_TODO("Extract to function.")
                            auto ids_count = static_cast<unsigned>(path.ids.size());

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

        const size_t yield_complex::COMPLEXITY_THRESHOLD = 1500;
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
            llvm::errs() << box_->getNameAsString() << "\n";
            cfg.dump(box_->getASTContext().getLangOpts(), false);

            cfg_data data(cfg);
            data.optimize();
            //            data.apply();
        }

    } // namespace

    basic_method* create_yield_complex()
    {
        return new methods::yield_complex;
    }

} // namespace
