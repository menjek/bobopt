/// \file bobopt_method.hpp File contains definition of interface for
/// an optimization method.

#ifndef BOBOPT_METHOD_HPP_GUARD_
#define BOBOPT_METHOD_HPP_GUARD_

#include <bobopt_inline.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/Tooling/Refactoring.h"
#include <clang/bobopt_clang_epilog.hpp>

// forward declarations:
namespace clang
{
    class CXXRecordDecl;
}

namespace bobopt
{

    // forward declarations:
    class optimizer;

    /// \brief Interface for any optimization method in the optimizer.
    /// Class also provides a method to access the optimizer main object.
    class basic_method
    {
    public:
        basic_method();
        virtual ~basic_method();

        /// \brief Pure virtual member function to handle optimization of a
        /// single box
        ///
        /// \param box_declaration Pointer to the AST node representing the box
        /// declaration
        /// \param replacements Pointer to the set of \code Replacemen objects
        /// used in \code RefactoringTool.
        virtual void optimize(clang::CXXRecordDecl* box_declaration, clang::tooling::Replacements* replacements) = 0;

    protected:
        /// \brief Acess to the optimizer main object.
        const optimizer& get_optimizer() const;

    private:
        friend class optimizer;

        const optimizer* optimizer_;
    };

} // namespace

#include BOBOPT_INLINE_IN_HEADER(bobopt_method.inl)

#endif // guard
