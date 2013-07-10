#ifndef BOBOPT_METHODS_BOBOPT_YIELD_COMPLEX_HPP_GUARD_
#define BOBOPT_METHODS_BOBOPT_YIELD_COMPLEX_HPP_GUARD_

#include <bobopt_method.hpp>

namespace bobopt {

	namespace methods {

		class yield_complex : public basic_method
		{
		public:
			virtual void optimize(clang::CXXRecordDecl* box_declaration, clang::tooling::Replacements* replacements) BOBOPT_OVERRIDE;
		};

	} // namespace methods

	basic_method* create_yield_complex();

} // namespace bobopt

#endif // guard