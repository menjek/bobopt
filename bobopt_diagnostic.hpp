/// \file bobopt_diagnostic.hpp Definition of class handling diagnostic messages.

#ifndef BOBOPT_DIAGNOSTIC_HPP_GUARD_
#define BOBOPT_DIAGNOSTIC_HPP_GUARD_

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
}

namespace bobopt {

class diagnostic_message
{
public:
	enum message_type
	{
		info,
		optimization,
		suggestion
	};

	diagnostic_message(message_type type,
		clang::SourceRange range,
		clang::SourceRange point_range,
		const std::string& point_message)

		: type_(type)
		, range_(range)
		, point_range_(point_range)
		, point_message_(point_message)
	{}

	message_type get_type() const
	{
		return type_;
	}

	clang::SourceRange get_range() const
	{
		return range_;
	}

	clang::SourceRange get_point_range() const
	{
		return point_range_;
	}

	std::string get_point_message() const
	{
		return point_message_;
	}

private:
	message_type type_;
	clang::SourceRange range_;
	clang::SourceRange point_range_;
	std::string point_message_;
};

class diagnostic
{
public:
	enum message_modes
	{
		pointers_only,
		all
	};

	explicit diagnostic(clang::CompilerInstance& compiler)
		: compiler_(compiler)
	{}

	void emit(const diagnostic_message& message, message_modes mode = pointers_only) const;

	diagnostic_message get_decl_diag_message(clang::Decl* decl, const std::string& message) const;

private:
	BOBOPT_NONCOPYMOVABLE(diagnostic);

	struct console_color
	{
		llvm::raw_ostream::Colors fg_color;
		bool bold;
	};

	static console_color s_location_color;
	static console_color s_pointers_color;
	static console_color s_info_color;
	static console_color s_suggestion_color;
	static console_color s_optimization_color;
	static console_color s_message_color;

	void emit_header(const diagnostic_message& message) const;
	void emit_pointers(const diagnostic_message& message, message_modes mode) const;

	clang::CompilerInstance& compiler_;
};

} // namespace

#endif // guard