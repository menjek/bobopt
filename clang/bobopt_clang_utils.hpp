/// \file bobopt_clang_utils.hpp Contains definitions of various utils extending clang functionality.

#ifndef BOBOPT_CLANG_BOBOPT_CLANG_UTILS_HPP_GUARD_
#define BOBOPT_CLANG_BOBOPT_CLANG_UTILS_HPP_GUARD_

#include <bobopt_language.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/RecursiveASTVisitor.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <string>
#include <vector>

namespace clang {
	namespace ast_matchers {
		class MatchFinder;
	}

	class ASTContext;
	class Decl;
	class Stmt;
	class Type;
	class Rewriter;
}

namespace bobopt {

	/// \brief Flushes all changes in rewriter object.
	void flush_rewriter(clang::Rewriter& rewriter);

	/// \brief Handles AST traversal and match finding.
	///
	/// By clang API, matchers can be used only once on whole translation unit and
	/// user can use matcher callback to refactor code. Sometimes user want to
	/// look for matching nodes in subtree of AST for single TU.
	///
	/// Clang AST matchers have \c match() member function that does what are matchers
	/// supposed to do, but for \b single node. Class combines them with
	/// RecursiveASTVisitor so client can match nodes in whole subtree.
	///
	/// \code
	/// MatchFinder finder;
	/// my_callback callback;
	/// finder.addMatcher(recordDecl(hasName("X")), &callback);
	/// recursive_match_finder recursive_finder(&finder, get_ast_context());
	/// finder.TraverseStmt(subtree);
	/// \endcode
	class recursive_match_finder : public clang::RecursiveASTVisitor<recursive_match_finder>
	{
	public:
		// create:
		recursive_match_finder(clang::ast_matchers::MatchFinder* match_finder, clang::ASTContext* context);

		// visit of base nodes:
		bool VisitDecl(clang::Decl* decl);
		bool VisitStmt(clang::Stmt* stmt);
		bool VisitType(clang::Type* type);

	private:

		// data members:
		clang::ast_matchers::MatchFinder* match_finder_;
		clang::ASTContext* context_;
	};

} // namespace

#endif // guard