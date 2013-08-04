#include <bobopt_diagnostic.hpp>

#include <bobopt_inline.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/Decl.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Lexer.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <cctype>
#include <iostream>
#include <string>

using namespace clang;
using namespace std;

namespace bobopt {

	namespace internal {

		BOBOPT_INLINE bool in_range(size_t begin, size_t end, size_t offset)
		{
			return (begin <= offset) && (offset < end);
		}
	
		string read_message_line(string& message)
		{
			size_t nl = message.find_first_of('\n');
			if (nl != string::npos)
			{
				string result = message.substr(0, nl);
				message = message.substr(nl + 1);
				return result;
			}
			
			string result = message;
			message.clear();
			return result;
		}

		string create_pointers_line(string line, size_t begin, size_t end)
		{
			for (size_t i = 0; i < line.size(); ++i)
			{
				if (in_range(begin, end, i))
				{
					line[i] = '^';
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

	void diagnostic::emit(diagnostic_message message, message_modes mode) const
	{
		SourceManager& source_manager = compiler_.getSourceManager();

		const char* range_begin = source_manager.getCharacterData(message.get_range().getBegin());
		const char* range_end = source_manager.getCharacterData(message.get_range().getEnd());
		
		size_t point_offset_begin = source_manager.getCharacterData(message.get_point_range().getBegin()) - range_begin;
		size_t point_offset_end = source_manager.getCharacterData(message.get_point_range().getEnd()) - range_begin;

		size_t offset_begin = 0;
		string range_string(range_begin, range_end);
		while (!range_string.empty())
		{
			string line = internal::read_message_line(range_string);
			size_t offset_end = offset_begin + line.size();
			
			if (internal::in_range(offset_begin, offset_end, point_offset_begin) || internal::in_range(offset_begin, offset_end, point_offset_end))
			{
				cout << line << endl;

				size_t pointers_begin = (point_offset_begin > offset_begin) ? (point_offset_begin - offset_begin) : 0;
				size_t pointers_end = (point_offset_end < offset_end) ? point_offset_end : offset_end;
				
				cout << internal::create_pointers_line(line, pointers_begin, pointers_end) << endl;
			}
			else
			{
				if (mode == range)
				{
					cout << line << endl;
				}
			}

			offset_begin += line.size() + 1;
		}
	}

	diagnostic_message diagnostic::get_decl_diag_message(Decl* decl, const string& message) const
	{
		SourceRange point_range(decl->getLocation(),
			Lexer::getLocForEndOfToken(decl->getLocation(),
				/* offset= */ 0,
				compiler_.getSourceManager(),
				compiler_.getLangOpts()));

		return diagnostic_message(decl->getSourceRange(), point_range, message);
	}

} // namespace