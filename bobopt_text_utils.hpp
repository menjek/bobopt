#ifndef BOBOPT_TEXT_UTILS_HPP_GUARD_
#define BOBOPT_TEXT_UTILS_HPP_GUARD_

#include <bobopt_inline.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/Basic/SourceLocation.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <string>

namespace clang
{
    class CXXRecordDecl;
    class Decl;
    class SourceManager;
    class Stmt;
}

namespace bobopt
{

    /// \brief Get indent string for specific location.
    std::string location_indent(const clang::SourceManager& sm, clang::SourceLocation location);
    /// \brief Get indent string for specific declaration. Function just forwards to \c location_indent.
    std::string decl_indent(const clang::SourceManager& sm, const clang::Decl* decl);
    /// \brief Get indent string for specific statement. Function just forwards to \c location_indent.
    std::string stmt_indent(const clang::SourceManager& sm, const clang::Stmt* stmt);

    /// \brief Representing detectable data within single box.
    struct document_indent
    {
        std::string line_;
        std::string method_;
        std::string endl_;
    };

    /// \brief Detect line indent in box.
    std::string detect_line_indent(clang::SourceManager& sm, const clang::CXXRecordDecl* decl);
    /// \brief Detect method indent in box.
    std::string detect_method_decl_indent(clang::SourceManager& sm, const clang::CXXRecordDecl* decl);
    /// \brief Detect line endings in box.
    std::string detect_line_end(clang::SourceManager& sm, const clang::CXXRecordDecl* decl);
    /// \brief Detect all the three above in single call.
    document_indent detect_document_indent(clang::SourceManager& sm, const clang::CXXRecordDecl* decl);

} // namespace

#include BOBOPT_INLINE_IN_HEADER(bobopt_text_utils)

#endif // guard