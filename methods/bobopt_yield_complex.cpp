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

        static const unsigned call_default_complexity = 200u;
        static const unsigned call_trivial_complexity = 25u;
        static const unsigned call_inline_complexity = 10u;
        static const unsigned call_constexpr_complexity = 1u;

        static const unsigned multiplier_for = 20u;
        static const unsigned multiplier_while = 25u;

        static const unsigned threshold_penalty = 8u;
        static const unsigned threshold = 1000u;

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
                bool yield;
                std::vector<unsigned> paths;
            };

            typedef std::unordered_map<unsigned, block_data> data_type;

        public:
            explicit cfg_data(const CFG& cfg) : cfg_(cfg)
            {
                cfg_builder builder;
                builder.process_cfg_block(data_, cfg.getEntry(), 0);
            }

            void optimize()
            {
                data_type temp(data_);
                float goodness = calc_goodness(data_);

                for (;;)
                {
                    optimize_step(temp, data_);
                    float temp_goodness = calc_goodness(temp);
                    if (temp_goodness < goodness)
                    {
                        break;
                    }

                    data_.swap(temp);
                    goodness = temp_goodness;
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
                explicit cfg_builder(bool build = true) : build_(build)
                {
                }

                void process_cfg_block(data_type& data, const CFGBlock& block, unsigned path_complexity)
                {
                    block_data& data_item = data[block.getBlockID()];

                    stack_scope<unsigned> guard(stack_, block.getBlockID());
                    if (guard.cycle())
                    {
                        // In loop?
                        CFGTerminator terminator = block.getTerminator();
                        if (terminator)
                        {
                            const Stmt* stmt = terminator.getStmt();
                            BOBOPT_ASSERT(stmt != nullptr);

                            if ((llvm::dyn_cast<ForStmt>(stmt) != nullptr) ||
                                (llvm::dyn_cast<WhileStmt>(stmt) != nullptr) ||
                                (llvm::dyn_cast<DoStmt>(stmt) != nullptr))
                            {
                                // Yep.
                                data_item.paths.push_back(path_complexity);
                                return;
                            }
                        }
                    }

                    if (build_)
                    {
                        data_item.yield = false;
                    }

                    unsigned block_complexity = 0u;
                    for (const CFGElement& element : block)
                    {
                        unsigned stmt_comlexity = calc_element_complexity(element);

                        if (stmt_comlexity == 0u)
                        {
                            BOBOPT_ASSERT(!build_ || data_item.yield);
                            block_complexity = 0u;
                            data_item.yield = true;
                            break;
                        }

                        if (!build_ && data_item.yield)
                        {
                            block_complexity = 0u;
                            break;
                        }

                        block_complexity += stmt_comlexity;
                    }

                    unsigned result_complexity = path_complexity + block_complexity;
                    data_item.paths.push_back(result_complexity);

                    process_succ(data, block, result_complexity);
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

                void process_succ(data_type& data, const CFGBlock& block, unsigned path_complexity)
                {
                    CFGTerminator terminator = block.getTerminator();
                    if (terminator)
                    {
                        const Stmt* stmt = terminator.getStmt();
                        BOBOPT_ASSERT(stmt != nullptr);

                        if (llvm::dyn_cast<ForStmt>(stmt) != nullptr)
                        {
                            process_succ_loop(data, block, path_complexity, multiplier_for);
                            return;
                        }

                        if (llvm::dyn_cast<WhileStmt>(stmt) != nullptr)
                        {
                            process_succ_loop(data, block, path_complexity, multiplier_while);
                            return;
                        }
                    }

                    for (auto it = block.succ_begin(), end = block.succ_end(); it != end; ++it)
                    {
                        process_cfg_block(data, **it, path_complexity);
                    }
                }

                void process_succ_loop(data_type& data, const CFGBlock& block, unsigned path_complexity, unsigned multiplier)
                {
                    auto it = block.succ_begin();
                    BOBOPT_ASSERT(it != block.succ_end());

                    const CFGBlock& body = **it;

                    ++it;
                    BOBOPT_ASSERT(it != block.succ_end());
                    const CFGBlock& skip = **it;

                    ++it;
                    BOBOPT_ASSERT(it == block.succ_end());

                    process_cfg_block(data, body, path_complexity);

                    block_data& data_item = data[block.getBlockID()];
                    for (auto& body_path : data_item.paths)
                    {
                        body_path *= multiplier;
                        body_path += path_complexity;
                        process_cfg_block(data, skip, body_path);
                    }
                }

                bool build_;
                std::vector<unsigned> stack_;
            };

            void optimize_step(data_type& new_data, const data_type& data)
            {
                BOBOPT_ASSERT(!data.empty() && (data.size() == new_data.size()));

                auto max_it = std::begin(data);
                float max_goodness = 0.0f;
                for (auto it = std::begin(data), end = std::end(data); it != end; ++it)
                {
                    unsigned sum = 0;
                    for (auto path : it->second.paths)
                    {
                        sum += path;
                    }

                    float goodness = sum / static_cast<float>(it->second.paths.size());
                    if (goodness > max_goodness)
                    {
                        max_it = it;
                        max_goodness = goodness;
                    }
                }

                auto new_it = new_data.find(max_it->first);
                BOBOPT_ASSERT(new_it != std::end(new_data));

                new_it->second.yield = true;

                for (auto data_pair : new_data)
                {
                    data_pair.second.paths.clear();
                }

                cfg_builder builder(false);
                builder.process_cfg_block(new_data, cfg_.getEntry(), 0);
            }

            float calc_goodness(const data_type& data) const
            {
                auto block_it = data.find(cfg_.getExit().getBlockID());
                BOBOPT_ASSERT(block_it != std::end(data));

                unsigned sum = 0;
                for (auto path : block_it->second.paths)
                {
                    if (path > threshold)
                    {
                        sum += threshold;
                        sum += (path - threshold) * threshold_penalty;
                    }
                    else
                    {
                        sum += path;
                    }
                }

                return sum / static_cast<float>(block_it->second.paths.size());
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
//            llvm::errs() << box_->getNameAsString() << "\n";
//            cfg.dump(box_->getASTContext().getLangOpts(), false);

            cfg_data data(cfg);
//             data.optimize();
//             data.apply();
        }

    } // namespace

    basic_method* create_yield_complex()
    {
        return new methods::yield_complex;
    }

} // namespace
