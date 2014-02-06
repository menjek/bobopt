#include <bobopt_inline.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/DeclBase.h"
#include "clang/AST/Stmt.h"
#include <clang/bobopt_clang_epilog.hpp>

namespace bobopt
{

    // Formatting.
    //==========================================================================

    BOBOPT_INLINE std::string decl_indent(const clang::SourceManager& sm, const clang::Decl* decl)
    {
        return location_indent(sm, decl->getLocStart());
    }

    BOBOPT_INLINE std::string stmt_indent(const clang::SourceManager& sm, const clang::Stmt* stmt)
    {
        return location_indent(sm, stmt->getLocStart());
    }

    BOBOPT_INLINE document_indent detect_document_indent(clang::SourceManager& sm, const clang::CXXRecordDecl* decl)
    {
        document_indent document;
        document.method_ = detect_method_decl_indent(sm, decl);
        document.line_ = detect_line_indent(sm, decl);
        document.endl_ = detect_line_end(sm, decl);
        return document;
    }

} // bobopt