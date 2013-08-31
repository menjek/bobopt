#include <methods/bobopt_yield_complex.hpp>

#include <bobopt_debug.hpp>
#include <bobopt_inline.hpp>
#include <bobopt_macros.hpp>
#include <bobopt_utils.hpp>
#include <clang/bobopt_clang_utils.hpp>
#include <methods/bobopt_complexity_tree.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <algorithm>

using namespace std;
using namespace clang;
using namespace clang::ast_type_traits;

namespace bobopt {

	namespace methods {
		
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
		void yield_complex::optimize(clang::CXXRecordDecl* box, clang::tooling::Replacements* replacements)
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
		///   - Create complexity tree.
		///   - Insert yield calls by analyzing complexity tree.
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

			complexity_ptr root = build_complexity_tree(body);
			insert_yields(body, root);
		}

		/// \brief Create complexity tree for function body.
		complexity_ptr yield_complex::build_complexity_tree(clang::CompoundStmt* compound_stmt) const
		{
			complexity_ptr root = compound_complexity::create(compound_stmt);
			BOBOPT_UNUSED_EXPRESSION(root);

			return root;
		}

		/// \brief Based on complexity tree insert yields into code.
		void yield_complex::insert_yields(clang::CompoundStmt* body, const complexity_ptr& root) const
		{
			BOBOPT_ASSERT(body != nullptr);
			BOBOPT_UNUSED_EXPRESSION(root);
		}

		// constants:

		/// \brief Constant that represents for loop with undetected number of body executions.
		const size_t yield_complex::FOR_UNKNOWN_LOOP_COUNT = static_cast<size_t>(-1);

		/// \brief Complexity of "too complex" for loop body, that will be using dynamic detection of complexity.
		const size_t yield_complex::FOR_RUNTIME_ANALYSIS_TRESHOLD = 300;

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