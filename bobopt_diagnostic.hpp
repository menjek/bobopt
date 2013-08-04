/// \file bobopt_diagnostic.hpp Definition of class handling diagnostic messages.

#ifndef BOBOPT_DIAGNOSTIC_HPP_GUARD_
#define BOBOPT_DIAGNOSTIC_HPP_GUARD_

#include <bobopt_macros.hpp>

#include <clang/bobopt_clang_prolog.hpp>
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
	diagnostic_message(clang::SourceRange range, clang::SourceRange point_range, const std::string& point_message)
		: range_(range)
		, point_range_(point_range)
		, point_message_(point_message)
	{}

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
	clang::SourceRange range_;
	clang::SourceRange point_range_;
	std::string point_message_;
};

class diagnostic
{
public:
	enum message_modes
	{
		single_line,
		range
	};

	explicit diagnostic(clang::CompilerInstance& compiler)
		: compiler_(compiler)
	{}

	void emit(diagnostic_message message, message_modes mode = single_line) const;

	diagnostic_message get_decl_diag_message(clang::Decl* decl, const std::string& message) const;

private:
	BOBOPT_NONCOPYMOVABLE(diagnostic);

	clang::CompilerInstance& compiler_;
};

} // namespace

#endif // guard