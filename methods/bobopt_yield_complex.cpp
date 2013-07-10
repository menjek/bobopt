#include <methods/bobopt_yield_complex.hpp>

#include <bobopt_macros.hpp>

namespace bobopt {

	namespace methods {

		void yield_complex::optimize(clang::CXXRecordDecl* box_declaration, clang::tooling::Replacements* replacements)
		{
			BOBOPT_UNUSED_EXPRESSION(box_declaration);
			BOBOPT_UNUSED_EXPRESSION(replacements);
		}

	} // namespace

	basic_method* create_yield_complex()
	{
		return new methods::yield_complex;
	}

} // namespace