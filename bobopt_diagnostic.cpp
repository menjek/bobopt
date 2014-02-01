#include <bobopt_diagnostic.hpp>

#include <bobopt_debug.hpp>
#include <bobopt_inline.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Lexer.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <cctype>
#include <string>

using namespace clang;

#include BOBOPT_INLINE_IN_SOURCE(bobopt_diagnostic)

namespace bobopt
{

    namespace detail
    {

        BOBOPT_INLINE bool in_range(size_t begin, size_t end, size_t offset)
        {
            return (begin <= offset) && (offset < end);
        }

        BOBOPT_INLINE std::string read_message_line(std::string& message)
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

        std::string create_pointers_line(std::string line, size_t begin, size_t end)
        {
            for (size_t i = 0; i < line.size(); ++i)
            {
                if (in_range(begin, end, i))
                {
                    line[i] = (i == begin) ? '^' : '~';
                }
                else
                {
                    if (!isspace(line[i]))
                    {
                        line[i] = ' ';
                    }
                }
            }

            return line;
        }
    }

    // diagnostic implementation.
    //==============================================================================

    diagnostic::console_color diagnostic::s_location_color = { llvm::raw_ostream::WHITE, true };

    diagnostic::console_color diagnostic::s_pointers_color = { llvm::raw_ostream::GREEN, true };

    diagnostic::console_color diagnostic::s_info_color = { llvm::raw_ostream::BLACK, true };

    diagnostic::console_color diagnostic::s_suggestion_color = { llvm::raw_ostream::MAGENTA, true };

    diagnostic::console_color diagnostic::s_optimization_color = { llvm::raw_ostream::RED, true };

    diagnostic::console_color diagnostic::s_message_color = { llvm::raw_ostream::WHITE, true };

    void diagnostic::emit(const source_message& message, source_modes mode) const
    {
        emit_header(message);
        emit_source(message, mode);
    }

    source_message diagnostic::get_message_decl(source_message::types type, const clang::Decl* decl, const std::string& message) const
    {
        SourceManager& source_manager = compiler_.getSourceManager();
        ;

        SourceLocation location = decl->getLocation();
        bool is_macro_expansion = source_manager.isMacroArgExpansion(location);

        SourceLocation last_expansion_location;
        while (source_manager.isMacroArgExpansion(location))
        {
            last_expansion_location = location;
            location = source_manager.getFileLoc(location);
        }

        SourceRange range = decl->getSourceRange();
        if (is_macro_expansion)
        {
            std::pair<SourceLocation, SourceLocation> expansion_range = source_manager.getExpansionRange(last_expansion_location);
            SourceLocation expansion_range_end = Lexer::getLocForEndOfToken(expansion_range.second,
                                                                            /* offset= */ 0,
                                                                            source_manager,
                                                                            compiler_.getLangOpts());
            range = SourceRange(expansion_range.first, expansion_range_end);
        }

        SourceLocation location_end = Lexer::getLocForEndOfToken(location,
                                                                 /* offset= */ 0,
                                                                 source_manager,
                                                                 compiler_.getLangOpts());

        return source_message(type, range, SourceRange(location, location_end), message);
    }

    source_message diagnostic::get_message_call_expr(source_message::types type, const CallExpr* call_expr, const std::string& message) const
    {
        return source_message(type, call_expr->getSourceRange(), call_expr->getSourceRange(), message);
    }

    void diagnostic::emit_header(const source_message& message) const
    {
        llvm::raw_ostream& out = llvm::outs();

        // Print location in sources.
        out.changeColor(s_location_color.fg_color, s_location_color.bold);
        out << message.get_point_range().getBegin().printToString(compiler_.getSourceManager()) << ": ";

        // Print message type.
        switch (message.get_type())
        {
        case source_message::info:
        {
            out.changeColor(s_info_color.fg_color, s_info_color.bold);
            out << "info: ";
            break;
        }

        case source_message::suggestion:
        {
            out.changeColor(s_suggestion_color.fg_color, s_suggestion_color.bold);
            out << "suggestion: ";
            break;
        }

        case source_message::optimization:
        {
            out.changeColor(s_optimization_color.fg_color, s_optimization_color.bold);
            out << "optimization: ";
            break;
        }

        default:
            BOBOPT_ERROR("unreachable_code");
        }

        // Print message.
        out.changeColor(s_message_color.fg_color, s_message_color.bold);
        out << message.get_message();
        out.resetColor();

        out << '\n';
    }

    void diagnostic::emit_source(const source_message& message, source_modes mode) const
    {
        SourceManager& source_manager = compiler_.getSourceManager();

        const char* range_begin = source_manager.getCharacterData(message.get_range().getBegin());
        const char* range_end = source_manager.getCharacterData(message.get_range().getEnd());

        size_t point_offset_begin = source_manager.getCharacterData(message.get_point_range().getBegin()) - range_begin;
        size_t point_offset_end = source_manager.getCharacterData(message.get_point_range().getEnd()) - range_begin;

        size_t offset_begin = 0;
        std::string range_string(range_begin, range_end);
        while (!range_string.empty())
        {
            std::string line = detail::read_message_line(range_string);
            size_t offset_end = offset_begin + line.size();

            if (detail::in_range(offset_begin, offset_end, point_offset_begin) || detail::in_range(offset_begin, offset_end, point_offset_end))
            {
                llvm::outs() << line << '\n';

                size_t pointers_begin = (point_offset_begin > offset_begin) ? (point_offset_begin - offset_begin) : 0;
                size_t pointers_end = (point_offset_end < offset_end) ? point_offset_end : offset_end;

                llvm::outs().changeColor(s_pointers_color.fg_color, s_pointers_color.bold);
                llvm::outs() << detail::create_pointers_line(line, pointers_begin, pointers_end) << '\n';
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