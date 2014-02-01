/// \brief bobopt_method.hpp Contains definition of interface for optimization method.

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

    class basic_method
    {
    public:
        basic_method();
        virtual ~basic_method();

        BOBOPT_INLINE const optimizer& get_optimizer() const;

        virtual void optimize(clang::CXXRecordDecl* box_declaration, clang::tooling::Replacements* replacements) = 0;

    private:
        friend class optimizer;

        BOBOPT_INLINE void set_optimizer(const optimizer* optimizer_instance);

        const optimizer* optimizer_;
    };

} // namespace

#include BOBOPT_INLINE_IN_HEADER(bobopt_method)

#endif // guard