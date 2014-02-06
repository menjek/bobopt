#include <clang/bobopt_clang_utils.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/DeclCXX.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/MemoryBuffer.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <string>

using namespace clang;
using namespace clang::ast_matchers;

#include BOBOPT_INLINE_IN_SOURCE(bobopt_clang_utils)

namespace bobopt
{

    bool overrides(const CXXMethodDecl* method_decl, const std::string& parent_name)
    {
        for (auto it = method_decl->begin_overridden_methods(); it != method_decl->end_overridden_methods(); ++it)
        {
            const CXXMethodDecl* overridden = *it;
            if (overridden->getParent()->getQualifiedNameAsString() == parent_name)
            {
                return true;
            }
        }

        return false;
    }

} // namespace