#include <bobopt_inline.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/DeclBase.h"
#include "clang/AST/Stmt.h"
#include <clang/bobopt_clang_epilog.hpp>

namespace bobopt
{

    // indent implementation.
    //==========================================================================

    BOBOPT_INLINE std::string decl_indent(const clang::SourceManager& sm, const clang::Decl* decl)
    {
        return location_indent(sm, decl->getLocStart());
    }

    BOBOPT_INLINE std::string stmt_indent(const clang::SourceManager& sm, const clang::Stmt* stmt)
    {
        return location_indent(sm, stmt->getLocStart());
    }

} // bobopt