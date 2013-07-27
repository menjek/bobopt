#include <methods/bobopt_yield_complex.hpp>

#include <bobopt_debug.hpp>
#include <bobopt_macros.hpp>
#include <clang/bobopt_clang_utils.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/RecursiveASTVisitor.h"
#include <clang/bobopt_clang_epilog.hpp>

using namespace std;
using namespace clang;

namespace bobopt {

	namespace methods {
		
		namespace internal {

			class complexity_evaluation_visitor : public RecursiveASTVisitor<complexity_evaluation_visitor>
			{
			public:

			private:

			};

		} // namespace internal

		yield_complex::yield_complex()
			: box_(nullptr)
			, replacements_(nullptr)
		{}

		yield_complex::~yield_complex()
		{}

		void yield_complex::optimize(clang::CXXRecordDecl* box, clang::tooling::Replacements* replacements)
		{
			BOBOPT_ASSERT(box != nullptr);
			BOBOPT_ASSERT(replacements != nullptr);

			box_ = box;
			replacements_ = replacements_;
			
			optimize_methods();
		}

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

		void yield_complex::optimize_method(exec_function_type method)
		{
			BOBOPT_UNUSED_EXPRESSION(method);
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