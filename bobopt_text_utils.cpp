#include <bobopt_text_utils.hpp>

#include <bobopt_inline.hpp>
#include <bobopt_macros.hpp>
#include <bobopt_optimizer.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/DeclCXX.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <string>

#include BOBOPT_INLINE_IN_SOURCE(bobopt_text_utils)

using namespace clang;

namespace bobopt
{

    // Input.
    //==========================================================================

    bool ask_yesno(const char* message)
    {
        std::string answer;
        while ((answer != "yes") && (answer != "no"))
        {
            llvm::outs() << message << " [yes/no]: ";
            llvm::outs().flush();
            std::cin >> answer;

            std::transform(std::begin(answer), std::end(answer), std::begin(answer), [](char c) { return static_cast<char>(tolower(c)); });
        }

        return answer == "yes";
    }

    // Formatting.
    //==========================================================================

    static const std::string default_indent = "\t";
    static const std::string unix_endl = "\n";
    static const std::string windows_endl = "\r\n";

    std::string location_indent(const SourceManager& sm, SourceLocation location)
    {
        auto* buffer = sm.getBuffer(sm.getFileID(location));
        const auto* bufferStart = buffer->getBufferStart();
        const auto* locationStart = sm.getCharacterData(location);

        const auto* lineStart = locationStart;
        while ((lineStart > bufferStart) && (*lineStart != '\n'))
        {
            --lineStart;
        }

        return std::string(lineStart + 1, locationStart);
    }

    std::string detect_line_indent(SourceManager& sm, const CXXRecordDecl* decl)
    {
        auto range = decl->getSourceRange();
        const char* first = sm.getCharacterData(range.getBegin());
        const char* last = sm.getCharacterData(range.getEnd());

        auto is_indent = [](char c) { return (c == ' ') || (c == '\t') || (c == '\r'); };

        std::map<std::string, unsigned> occurrences;

        std::string last_indent;
        while (first < last)
        {
            auto line_start = std::find_if_not(first, last, is_indent);
            if (line_start == last)
            {
                break;
            }

            auto line_end = std::find(line_start, last, '\n');
            if (line_start == line_end)
            {
                first = line_end + 1;
                continue;
            }

            std::string line_indent(first, line_start);
            if (last_indent.size() > line_indent.size())
            {
                if (last_indent.substr(0, line_indent.size()) == line_indent)
                {
                    auto indent = last_indent.substr(line_indent.size());
                    ++occurrences[indent];
                    last_indent = line_indent;
                }
            }
            else if (line_indent.size() > last_indent.size())
            {
                if (line_indent.substr(0, last_indent.size()) == last_indent)
                {
                    auto indent = line_indent.substr(last_indent.size());
                    ++occurrences[indent];
                    last_indent = line_indent;
                }
            }

            first = line_end + 1;
        }

        // If there's not at least very minimal differences between two
        // following lines, choose tabs. It won't break anything, code
        // is already messy anyway.
        if (occurrences.empty())
        {
            return default_indent;
        }

        auto max_indent = std::max_element(
            std::begin(occurrences),
            std::end(occurrences),
            [](const std::pair<std::string, unsigned> & lhs, const std::pair<std::string, unsigned> & rhs) { return lhs.second < rhs.second; });

        return max_indent->first;
    }

    std::string detect_method_decl_indent(SourceManager& sm, const CXXRecordDecl* decl)
    {
        std::map<std::string, unsigned> occurrences;
        for (auto it = decl->method_begin(), end = decl->method_end(); it != end; ++it)
        {
            const CXXMethodDecl* method_decl = *it;
            if (method_decl->isUserProvided())
            {
                auto indent = decl_indent(sm, method_decl);
                auto found = occurrences.find(indent);
                if (found == std::end(occurrences))
                {
                    occurrences[indent] = 0;
                }
                else
                {
                    ++(found->second);
                }
            }
        }

        BOBOPT_ASSERT(!occurrences.empty());

        auto max_indent = std::max_element(
            std::begin(occurrences),
            std::end(occurrences),
            [](const std::pair<std::string, unsigned> & lhs, const std::pair<std::string, unsigned> & rhs) { return lhs.second < rhs.second; });

        return max_indent->first;
    }

    std::string detect_line_end(clang::SourceManager& sm, const clang::CXXRecordDecl* decl)
    {
        const auto location = decl->getLocation();
        const char* data = sm.getCharacterData(location);

        auto* buffer = sm.getBuffer(sm.getFileID(location));
        const char* failsafe = buffer->getBufferEnd();

        const char* endl = std::find(data + 1, failsafe, '\n');
        if (endl == failsafe)
        {
            return unix_endl;
        }

        --endl;
        if (*endl == '\r')
        {
            return windows_endl;
        }

        return unix_endl;
    }

    document_indent detect_document_indent(clang::SourceManager& sm, const clang::CXXRecordDecl* decl)
    {
        document_indent document;
        document.method_ = detect_method_decl_indent(sm, decl);
        document.line_ = detect_line_indent(sm, decl);
        document.endl_ = detect_line_end(sm, decl);
        return document;
    }

} // namespace
