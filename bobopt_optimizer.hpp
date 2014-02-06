/// \file bobopt_optimizer.hpp Contains base class for bobox optimizations.

#ifndef BOBOPT_OPTIMIZER_HPP_GUARD_
#define BOBOPT_OPTIMIZER_HPP_GUARD_

#include <bobopt_diagnostic.hpp>
#include <bobopt_inline.hpp>
#include <bobopt_language.hpp>
#include <bobopt_method.hpp>
#include <bobopt_method_factory.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Refactoring.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <array>
#include <memory>

// Forward declaration(s).
namespace clang
{
    class CXXRecordDecl;
    class CompilerInstance;
}

namespace bobopt
{

    /// \brief Optimization level type.
    enum levels
    {
        OL_NONE = 0,
        OL_BASIC = 1,
        OL_EXTRA = 2,

        OL_COUNT
    };

    /// \brief Optimization modes.
    enum modes
    {
        MODE_DIAGNOSTIC,
        MODE_INTERACTIVE,
        MODE_BUILD
    };

    /// \brief Base class for bobox optimizations.
    ///
    /// Inherited from clang ast match finder callback. It also contains
    /// definition of matcher for finding bobox boxes.
    class optimizer : public clang::ast_matchers::MatchFinder::MatchCallback
    {
    public:

        static const clang::ast_matchers::DeclarationMatcher BOBOX_BOX_MATCHER;
        static const clang::ast_matchers::DeclarationMatcher BOBOX_BASIC_BOX_MATCHER;
        static const clang::ast_matchers::DeclarationMatcher USER_BOX_MATCHER;

        optimizer(modes mode, clang::tooling::Replacements* replacements);
        optimizer(modes mode, clang::tooling::Replacements* replacements, levels level);

        template <typename InputIterator>
        optimizer(clang::tooling::Replacements* replacements, InputIterator first, InputIterator last);

        ~optimizer();

        void set_level(levels level);
        modes get_mode() const;
        bool verbose() const;

        diagnostic& get_diagnostic();
        const diagnostic& get_diagnostic() const;

        void enable_method(method_type method);
        void disable_method(method_type method);
        bool is_method_enabled(method_type method) const;

        clang::CompilerInstance& get_compiler() const;
        void set_compiler(clang::CompilerInstance* compiler);

        clang::CXXRecordDecl* get_bobox_box() const;
        clang::CXXRecordDecl* get_bobox_basic_box() const;

        virtual void run(const clang::ast_matchers::MatchFinder::MatchResult& result) BOBOPT_OVERRIDE;

    private:
        typedef const method_type* method_iterator;
        typedef std::pair<method_iterator, method_iterator> method_iterator_pair;

        template <typename InputIterator>
        void construct(InputIterator first, InputIterator last);

        void create_method(method_type method);
        void destroy_method(method_type method);

        void apply_methods(clang::CXXRecordDecl* box_decl) const;

        static method_iterator_pair get_level_methods(levels level);

        modes mode_;
        clang::CXXRecordDecl* bobox_box_;
        clang::CXXRecordDecl* bobox_basic_box_;
        clang::CompilerInstance* compiler_;
        clang::tooling::Replacements* replacements_;
        std::unique_ptr<diagnostic> diagnostic_;
        std::array<basic_method*, OM_COUNT> methods_;
    };

} // namespace

#include BOBOPT_INLINE_IN_HEADER(bobopt_optimizer)

#endif // guard