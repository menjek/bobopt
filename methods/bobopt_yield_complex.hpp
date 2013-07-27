#ifndef BOBOPT_METHODS_BOBOPT_YIELD_COMPLEX_HPP_GUARD_
#define BOBOPT_METHODS_BOBOPT_YIELD_COMPLEX_HPP_GUARD_

#include <bobopt_method.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/Tooling/Refactoring.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <string>

// forward declarations:
namespace clang {
	class CXXRecordDecl;
	class CXXMethodDecl;
}

namespace bobopt {

	namespace methods {

		class yield_complex : public basic_method
		{
		public:
		
			// create/destroy:
			yield_complex();
			virtual ~yield_complex() BOBOPT_OVERRIDE;

			// optimize:
			virtual void optimize(clang::CXXRecordDecl* box, clang::tooling::Replacements* replacements) BOBOPT_OVERRIDE;

		private:
			
			// helpers:
			struct method_override
			{
				std::string method_name;
				std::string parent_name;
			};

			// typedefs:
			typedef clang::CXXMethodDecl* exec_function_type;

			// helpers:
			void optimize_methods();
			void optimize_method(exec_function_type method);

			// data members:
			clang::CXXRecordDecl* box_;
			clang::tooling::Replacements* replacements_;

			// constants.
			static const size_t BOX_EXEC_METHOD_COUNT = 3;
			static const method_override BOX_EXEC_METHOD_OVERRIDES[BOX_EXEC_METHOD_COUNT];
		};

	} // namespace methods

	basic_method* create_yield_complex();

} // namespace bobopt

#endif // guard