#include <methods/bobopt_yield_complex.hpp>

#include <bobopt_debug.hpp>
#include <bobopt_inline.hpp>
#include <bobopt_macros.hpp>
#include <bobopt_utils.hpp>
#include <clang/bobopt_clang_utils.hpp>
#include <methods/bobopt_complexity_tree.hpp>

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

        static const unsigned threshold_penalty = 8u;
        static const unsigned threshold = 250000u;

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
                    unsigned id;
                    unsigned complexity;
                };

                bool yield;
                std::vector<path_data> paths;
                std::vector<path_data> temps;
            };

            typedef std::unordered_map<unsigned, block_data> data_type;

        public:
            explicit cfg_data(const CFG& cfg) : cfg_(cfg)
            {
                cfg_builder builder;
                builder.process(data_, cfg.getEntry());
            }

            void optimize()
            {
                data_type temp(data_);
                float goodness = calc_goodness(data_);

                for (;;)
                {
                    if (!optimize_step(temp, data_))
                    {
                        break;
                    }

                    float temp_goodness = calc_goodness(temp);
                    if (temp_goodness > goodness)
                    {
                        data_.swap(temp);
                        goodness = temp_goodness;
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
                    block_data::path_data entry_path;
                    entry_path.id = next_id();
                    entry_path.complexity = 0;
                    process_cfg_block(data, entry_block, entry_path);
                }

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

                void process_cfg_block(data_type& data, const CFGBlock& block, block_data::path_data path)
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
                                block_item.temps.push_back(path);
                                return;
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
                            BOBOPT_ASSERT(!build_ || block_item.yield);
                            block_item.yield = true;
                            break;
                        }

                        if (!build_ && block_item.yield)
                        {
                            break;
                        }
                    }

                    block_data::path_data result_path;
                    result_path.id = path.id;
                    result_path.complexity = path.complexity + block_complexity;
                    block_item.paths.push_back(result_path);

                    if (block_item.yield)
                    {
                        result_path.id = next_id();
                        result_path.complexity = 0;
                        block_item.paths.push_back(result_path);
                    }

                    process_succ(data, block, result_path);
                }

                void process_succ(data_type& data, const CFGBlock& block, block_data::path_data path)
                {
                    CFGTerminator terminator = block.getTerminator();
                    if (terminator)
                    {
                        const Stmt* stmt = terminator.getStmt();
                        BOBOPT_ASSERT(stmt != nullptr);

                        if (llvm::dyn_cast<ForStmt>(stmt) != nullptr)
                        {
                            process_succ_loop(data, block, path, multiplier_for);
                            return;
                        }

                        if (llvm::dyn_cast<WhileStmt>(stmt) != nullptr)
                        {
                            process_succ_loop(data, block, path, multiplier_while);
                            return;
                        }
                    }

                    auto block_it = block.succ_begin();
                    if (block_it != block.succ_end())
                    {
                        process_cfg_block(data, **block_it, path);

                        ++block_it;
                        for (auto end = block.succ_end(); block_it != end; ++block_it)
                        {
                            block_data::path_data new_path;
                            new_path.id = next_id();
                            new_path.complexity = path.complexity;
                            process_cfg_block(data, **block_it, new_path);
                        }
                    }
                }

                void process_succ_loop(data_type& data, const CFGBlock& block, block_data::path_data path, unsigned multiplier)
                {
                    BOBOPT_ASSERT(data.count(block.getBlockID()) == 1);

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
                    {
                        block_data::path_data body_path;
                        body_path.id = path.id;
                        body_path.complexity = 0u;
                        process_cfg_block(data, body, body_path);
                    }

                    block_data& data_item = data[block.getBlockID()];
                    for (auto body_path : data_item.temps)
                    {
                        body_path.complexity *= multiplier;
                        body_path.complexity += path.complexity;
                        process_cfg_block(data, skip, body_path);
                    }

                    data_item.temps.clear();

                    // Paths that entered for already left from temps.
                    // Their "skip body" version needs new id.
                    for (auto& skip_path : data_item.paths)
                    {
                        skip_path.id = next_id();
                        process_cfg_block(data, skip, skip_path);
                    }
                }

                bool build_;
                unsigned max_id_;
                std::vector<unsigned> stack_;
            };

            bool optimize_step(data_type& new_data, const data_type& data)
            {
                BOBOPT_ASSERT(!data.empty() && (data.size() == new_data.size()));
                BOBOPT_TODO("Implement from scratch.");
                return false;
            }

            float calc_goodness(const data_type& data) const
            {
                auto block_it = data.find(cfg_.getExit().getBlockID());
                BOBOPT_ASSERT(block_it != std::end(data));

                // Paths ending in EXIT block.
                unsigned sum = 0;
                for (auto path : block_it->second.paths)
                {
                    if (path.complexity > threshold)
                    {
                        sum += threshold;
                        sum += (path.complexity - threshold) * threshold_penalty;
                    }
                    else
                    {
                        sum += path.complexity;
                    }
                }

                size_t paths_count = block_it->second.paths.size();

                // Paths ending in yielded blocks.
                for (const auto& block_pair : data)
                {
                    if (block_pair.second.yield)
                    {
                        sum += std::accumulate(std::begin(block_pair.second.paths),
                                               std::end(block_pair.second.paths),
                                               0u,
                                               [](unsigned int lhs, const block_data::path_data & rhs) { return lhs + rhs.complexity; });
                        paths_count += block_pair.second.paths.size();
                    }
                }

                return sum / static_cast<float>(paths_count);
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
