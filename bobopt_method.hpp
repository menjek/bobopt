/// \brief bobopt_method.hpp Contains definition of interface for optimization method.

#ifndef BOBOPT_METHOD_HPP_GUARD_
#define BOBOPT_METHOD_HPP_GUARD_

#include <bobopt_inline.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/Tooling/Refactoring.h"
#include <clang/bobopt_clang_epilog.hpp>

namespace clang {
	class ASTContext;
	class CXXRecordDecl;
}

namespace bobopt {

class basic_method
{
public:
	basic_method();
	virtual ~basic_method();

	BOBOPT_INLINE void set_ast_context(clang::ASTContext* context);
	BOBOPT_INLINE clang::ASTContext* get_ast_context() const;

	virtual void optimize(clang::CXXRecordDecl* box_declaration, clang::tooling::Replacements* replacements) = 0;

private:
	clang::ASTContext* context_;
};

} // namespace

#include BOBOPT_INLINE_IN_HEADER(bobopt_method)

#endif // guard