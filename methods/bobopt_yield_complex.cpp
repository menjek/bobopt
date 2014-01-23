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
#include <vector>
#include <unordered_map>

using namespace std;
using namespace clang;
using namespace clang::ast_type_traits;

namespace bobopt {

	namespace methods {

        // helpers implementation.
		//==============================================================================

        static bool has_yield(const CFGElement& element)
        {
            if (element.getKind() != CFGElement::Kind::Statement)
            {
                return false;
            }

            const Stmt* stmt = element.castAs<CFGStmt>().getStmt();
            BOBOPT_ASSERT(stmt != nullptr);

            ast_nodes_collector<CXXMemberCallExpr> collector;
            collector.TraverseStmt(const_cast<Stmt*>(stmt));

            for (auto it = collector.nodes_begin(), end = collector.nodes_end(); it != end; ++it)
            {
                const CXXMethodDecl* method_decl = (*it)->getMethodDecl();
                const CXXRecordDecl* record_decl = (*it)->getRecordDecl();

                if ((method_decl->getNameAsString() == "yield") && (record_decl->getNameAsString() == "basic_box"))
                {
                    return true;
                }
            }

            return false;
        }

        static unsigned calc_stmt_complexity(const CFGElement& element)
        {
            return 0;
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
                builder.process_cfg_block(cfg.getEntry(), 0);
                data_ = builder.get_data();
            }

            void optimize()
            {
                data_type temp;
                unsigned goodness = calc_goodness(data_);

                for (;;)
                {
                    optimize_impl(temp, data_);
                    unsigned temp_goodness = calc_goodness(temp);
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
            }

        private:
            BOBOPT_NONCOPYMOVABLE(cfg_data);

            class cfg_builder
            {
            public:
                void process_cfg_block(const CFGBlock& block, unsigned int path_complexity)
                {
                    stack_scope<unsigned> guard(stack_, block.getBlockID());
                    if (guard.cycle())
                    {
                        return;
                    }

                    block_data& data = data_[block.getBlockID()];
                    data.yield = false;

                    unsigned int block_complexity = 0;
                    for (const CFGElement& element : block)
                    {
                        if (has_yield(element))
                        {
                            data.yield = true;
                            break;
                        }
        
                        block_complexity += calc_stmt_complexity(element);
                    }

                    unsigned int result_complexity = path_complexity + block_complexity;
                    data.paths.push_back(result_complexity);

                    process_succ(block, result_complexity);
                }

                std::unordered_map<unsigned, block_data> get_data() const
                {
                    return data_;
                }

            private:
                template<typename T>
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

                void process_succ(const CFGBlock& block, unsigned path_complexity)
                {
                    for (auto it = block.succ_begin(), end = block.succ_end(); it != end; ++it)
                    {
                        process_cfg_block(**it, path_complexity);
                    }
                }

                data_type data_;
                std::vector<unsigned> stack_;
            };

            static void optimize_impl(data_type& new_data, const data_type& data)
            {
                BOBOPT_UNUSED_EXPRESSION(new_data);
                BOBOPT_UNUSED_EXPRESSION(data);
            }

            static unsigned calc_goodness(const data_type& data)
            {
                BOBOPT_UNUSED_EXPRESSION(data);
                return 0;
            }

            const CFG& cfg_;
            data_type data_;
        };

		// yield_complex implementation.
		//==============================================================================

		/// \brief Create default constructed unusable object.
		yield_complex::yield_complex()
			: box_(nullptr)
			, replacements_(nullptr)
		{}

		/// \brief Deletable through pointer to base.
		yield_complex::~yield_complex()
		{}

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
		/// Function iterates through box methods and if it matches method in array it calls dedicated function to optimizer single method.
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
        /// Optimization is done in 2 steps:
        ///   - Construct CFG.
        ///   - Insert yield calls by analyzing CFG.
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
            CFG* cfg = CFG::buildCFG(method, body, &method->getASTContext(), options);

            if (cfg == nullptr)
            {
                BOBOPT_ERROR("Failed to construct CFG.");
                return;
            }

            optimize_body(*cfg);
            delete cfg;
        }

        void yield_complex::optimize_body(const CFG& cfg)
        {
            cfg_data data(cfg);
            data.optimize();
            data.apply();
        }

        // constants:

		const yield_complex::method_override yield_complex::BOX_EXEC_METHOD_OVERRIDES[BOX_EXEC_METHOD_COUNT] =
		{
			{ {"sync_mach_etwas"}, {"bobox::basic_box"} },
			{ {"async_mach_etwas"}, {"bobox::basic_box"} },
			{ {"body_mach_etwas"}, {"bobox::basic_box"} }
		};

	} // namespace

	basic_method* create_yield_complex()
	{
		return new methods::yield_complex;
	}

} // namespace
