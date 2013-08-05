/// \file bobopt_diagnostic.hpp Definition of class handling diagnostic messages.

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
namespace clang {
	class CompilerInstance;
	class Decl;
	class CallExpr;
}

namespace bobopt {

	class source_message
	{
	public:
		enum types
		{
			info,
			optimization,
			suggestion
		};

		// create:
		source_message(types type,
			clang::SourceRange range,
			clang::SourceRange point_range,
			const std::string& message);

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

	class diagnostic
	{
	public:
		enum source_modes
		{
			pointers_only,
			dump
		};

		// create:
		explicit diagnostic(clang::CompilerInstance& compiler);

		// message emition.
		void emit(const source_message& message, source_modes mode = pointers_only) const;

		// source_message creation.
		source_message get_message_decl(source_message::types type, clang::Decl* decl, const std::string& message) const;
		source_message get_message_call_expr(source_message::types type, clang::CallExpr* call_expr, const std::string& message) const;

	private:
		BOBOPT_NONCOPYMOVABLE(diagnostic);

		struct console_color
		{
			llvm::raw_ostream::Colors fg_color;
			bool bold;
		};

		void emit_header(const source_message& message) const;
		void emit_source(const source_message& message, source_modes mode) const;

		static console_color s_location_color;
		static console_color s_pointers_color;
		static console_color s_info_color;
		static console_color s_suggestion_color;
		static console_color s_optimization_color;
		static console_color s_message_color;

		clang::CompilerInstance& compiler_;
	};

} // namespace

#include BOBOPT_INLINE_IN_HEADER(bobopt_diagnostic)

#endif // guard