#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/DeclBase.h"
#include "clang/AST/Stmt.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <clang/bobopt_clang_epilog.hpp>

namespace clang
{
    class ASTContext;
    class Decl;
    class Stmt;
    class Type;
}

namespace bobopt
{

    // recursive_match_finder implementation.
    //==========================================================================

    /// \brief Construction of match finder with necessay data.
    BOBOPT_INLINE recursive_match_finder::recursive_match_finder(clang::ast_matchers::MatchFinder* match_finder, clang::ASTContext* context)
        : match_finder_(match_finder)
        , context_(context)
    {
    }

    /// \brief Try to match \c clang::Decl.
    BOBOPT_INLINE bool recursive_match_finder::VisitDecl(clang::Decl* decl)
    {
        match_finder_->match(*decl, *context_);
        return true;
    }

    /// \brief Try to match \c clang::Stmt.
    BOBOPT_INLINE bool recursive_match_finder::VisitStmt(clang::Stmt* stmt)
    {
        match_finder_->match(*stmt, *context_);
        return true;
    }

    /// \brief Try to match \c clang::Type.
    BOBOPT_INLINE bool recursive_match_finder::VisitType(clang::Type* type)
    {
        match_finder_->match(*type, *context_);
        return true;
    }

} // namespace