#include <bobopt_diagnostic.hpp>

#include <bobopt_debug.hpp>
#include <bobopt_inline.hpp>
#include <bobopt_utils.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Lexer.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <algorithm>
#include <cctype>
#include <string>

using namespace clang;

#include BOBOPT_INLINE_IN_SOURCE(bobopt_diagnostic.inl)

namespace bobopt
{

    // helpers.
    //==========================================================================

    /// \brief Read line from string and update this string.
    static BOBOPT_INLINE std::string read_message_line(std::string& message)
    {
        size_t nl = message.find_first_of('\n');
        if (nl != std::string::npos)
        {
            std::string result = message.substr(0, nl);
            message = message.substr(nl + 1);
            return result;
        }

        std::string result = message;
        message.clear();
        return result;
    }

    /// \brief Build line of pointers for message.
    static std::string build_pointers_line(size_t begin, size_t end)
    {
        BOBOPT_ASSERT(begin < end);

        std::string result(end, ' ');

        result[begin] = '^';
        std::fill_n(result.begin() + begin + 1, end - begin - 1, '~');

        return result;
    }

    // diagnostic implementation.
    //==========================================================================

    const diagnostic::console_color diagnostic::LOCATION_COLOR = { llvm::raw_ostream::WHITE, true };
    const diagnostic::console_color diagnostic::POINTERS_COLOR = { llvm::raw_ostream::GREEN, true };
    const diagnostic::console_color diagnostic::INFO_COLOR = { llvm::raw_ostream::BLACK, true };
    const diagnostic::console_color diagnostic::SUGGESTION_COLOR = { llvm::raw_ostream::MAGENTA, true };
    const diagnostic::console_color diagnostic::OPTIMIZATION_COLOR = { llvm::raw_ostream::RED, true };
    const diagnostic::console_color diagnostic::MESSAGE_COLOR = { llvm::raw_ostream::WHITE, true };

    const size_t diagnostic::MIN_DESIRED_MESSAGE_SIZE = 50;

    /// \brief Emit diagnostic message in desired mode.
    void diagnostic::emit(const diagnostic_message& message, source_modes mode) const
    {
        emit_header(message);
        emit_source(message, mode);
    }

    /// \brief Create diagnostic message for desired declaration.
    diagnostic_message diagnostic::get_message_decl(diagnostic_message::types type, const clang::Decl* decl, const std::string& message) const
    {
        SourceManager& sm = compiler_.getSourceManager();

        SourceLocation location = decl->getLocation();
        bool is_macro_expansion = sm.isMacroArgExpansion(location);

        SourceLocation last_expansion_location;
        while (sm.isMacroArgExpansion(location))
        {
            last_expansion_location = location;
            location = sm.getFileLoc(location);
        }

        SourceRange range = decl->getSourceRange();
        if (is_macro_expansion)
        {
            std::pair<SourceLocation, SourceLocation> expansion_range = sm.getExpansionRange(last_expansion_location);
            SourceLocation expansion_range_end = Lexer::getLocForEndOfToken(expansion_range.second,
                                                                            /* offset= */ 0,
                                                                            sm,
                                                                            compiler_.getLangOpts());
            range = SourceRange(expansion_range.first, expansion_range_end);
        }

        SourceLocation location_end = Lexer::getLocForEndOfToken(location,
                                                                 /* offset= */ 0,
                                                                 sm,
                                                                 compiler_.getLangOpts());

        return diagnostic_message(type, range, SourceRange(location, location_end), message);
    }

    /// \brief Create diagnostic message for desired statement.
    diagnostic_message diagnostic::get_message_stmt(diagnostic_message::types type, const Stmt* stmt, const std::string& message) const
    {
        return diagnostic_message(type, stmt->getSourceRange(), stmt->getSourceRange(), message);
    }

    /// \brief Emit header message with source code location, type and user defined message.
    void diagnostic::emit_header(const diagnostic_message& message) const
    {
        llvm::raw_ostream& out = llvm::outs();

        // Print location in sources.
        out.changeColor(LOCATION_COLOR.fg_color, LOCATION_COLOR.bold);
        out << message.get_point_range().getBegin().printToString(compiler_.getSourceManager()) << ": ";

        // Print message type.
        switch (message.get_type())
        {
        case diagnostic_message::info:
        {
            out.changeColor(INFO_COLOR.fg_color, INFO_COLOR.bold);
            out << "info: ";
            break;
        }

        case diagnostic_message::suggestion:
        {
            out.changeColor(SUGGESTION_COLOR.fg_color, SUGGESTION_COLOR.bold);
            out << "suggestion: ";
            break;
        }

        case diagnostic_message::optimization:
        {
            out.changeColor(OPTIMIZATION_COLOR.fg_color, OPTIMIZATION_COLOR.bold);
            out << "optimization: ";
            break;
        }

        default:
            BOBOPT_ERROR("unreachable_code");
        }

        // Print message.
        out.changeColor(MESSAGE_COLOR.fg_color, MESSAGE_COLOR.bold);
        out << message.get_message();
        out.resetColor();

        out << '\n';
    }

    /// \brief Emit source code message part.
    void diagnostic::emit_source(const diagnostic_message& message, source_modes mode) const
    {
        auto& sm = compiler_.getSourceManager();

        auto range = message.get_range();
        const char* range_begin = sm.getCharacterData(range.getBegin());
        const char* range_end = sm.getCharacterData(range.getEnd());

        if ((range_end - range_begin) < static_cast<ptrdiff_t>(MIN_DESIRED_MESSAGE_SIZE))
        {
            auto* buffer = sm.getBuffer(sm.getFileID(range.getEnd()));
            auto line_end = std::find(range_end, buffer->getBufferEnd(), '\n');
            range_end = std::min(range_begin + MIN_DESIRED_MESSAGE_SIZE, line_end);
        }

        auto point_range = message.get_point_range();
        size_t point_offset_begin = sm.getCharacterData(point_range.getBegin()) - range_begin;
        size_t point_offset_end = sm.getCharacterData(point_range.getEnd()) - range_begin;

        size_t offset_begin = 0;
        std::string range_string(range_begin, range_end);
        while (!range_string.empty())
        {
            std::string line = read_message_line(range_string);
            size_t offset_end = offset_begin + line.size();

            if (in_range(offset_begin, offset_end, point_offset_begin) || in_range(offset_begin, offset_end, point_offset_end))
            {
                llvm::outs() << line << '\n';

                size_t pointers_begin = (point_offset_begin > offset_begin) ? (point_offset_begin - offset_begin) : 0;
                size_t pointers_end = (point_offset_end < offset_end) ? point_offset_end : offset_end;

                llvm::outs().changeColor(POINTERS_COLOR.fg_color, POINTERS_COLOR.bold);
                llvm::outs() << build_pointers_line(pointers_begin, pointers_end) << '\n';
                llvm::outs().resetColor();
            }
            else
            {
                if (mode == dump)
                {
                    llvm::outs() << line << '\n';
                }
            }

            offset_begin += line.size() + 1;
        }
    }

} // namespace
