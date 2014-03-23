/// \file bobopt_diagnostic.hpp Definition of class handling diagnostic
/// messages.

#ifndef BOBOPT_DIAGNOSTIC_HPP_GUARD_
#define BOBOPT_DIAGNOSTIC_HPP_GUARD_

#include <bobopt_inline.hpp>
#include <bobopt_macros.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "llvm/Support/raw_ostream.h"
#include "clang/Basic/SourceLocation.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <string>

// forward declarations:
namespace clang
{
    class CompilerInstance;
    class Decl;
    class Stmt;
}

namespace bobopt
{

    /// \brief Holder of all neccessary information to print (Clang-like)
    /// diagnostic message with piece of source code.
    class diagnostic_message
    {
    public:
        /// \brief Supported message types.
        enum types
        {
            info,
            optimization,
            suggestion
        };

        // create:
        diagnostic_message(types type, clang::SourceRange range, clang::SourceRange point_range, const std::string& message);

        // access:
        types get_type() const;
        clang::SourceRange get_range() const;
        clang::SourceRange get_point_range() const;
        std::string get_message() const;

    private:
        // data members:
        types type_;
        clang::SourceRange range_;
        clang::SourceRange point_range_;
        std::string message_;
    };

    /// \brief Class responsible for printing diagnostic to application output.
    class diagnostic
    {
    public:
        /// \brief Modes for printing source code.
        enum source_modes
        {
            pointers_only,
            dump
        };

        // create:
        explicit diagnostic(clang::CompilerInstance& compiler);

        // message emition:
        void emit(const diagnostic_message& message, source_modes mode = pointers_only) const;

        // source_message creation:
        diagnostic_message get_message_decl(diagnostic_message::types type, const clang::Decl* decl, const std::string& message) const;
        diagnostic_message get_message_stmt(diagnostic_message::types type, const clang::Stmt* stmt, const std::string& message) const;

    private:
        BOBOPT_NONCOPYMOVABLE(diagnostic);

        struct console_color
        {
            llvm::raw_ostream::Colors fg_color;
            bool bold;
        };

        void emit_header(const diagnostic_message& message) const;
        void emit_source(const diagnostic_message& message, source_modes mode) const;

        // data members:
        clang::CompilerInstance& compiler_;

        // constants:
        static const console_color LOCATION_COLOR;
        static const console_color POINTERS_COLOR;
        static const console_color INFO_COLOR;
        static const console_color SUGGESTION_COLOR;
        static const console_color OPTIMIZATION_COLOR;
        static const console_color MESSAGE_COLOR;

        static const size_t MIN_DESIRED_MESSAGE_SIZE;
    };

} // namespace

#include BOBOPT_INLINE_IN_HEADER(bobopt_diagnostic.inl)

#endif // guard
