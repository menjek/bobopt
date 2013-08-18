/// \file bobopt_clang_utils.hpp Contains definitions of various utils extending clang functionality.

#ifndef BOBOPT_CLANG_BOBOPT_CLANG_UTILS_HPP_GUARD_
#define BOBOPT_CLANG_BOBOPT_CLANG_UTILS_HPP_GUARD_

#include <bobopt_debug.hpp>
#include <bobopt_inline.hpp>
#include <bobopt_language.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "llvm/Support/Casting.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <iterator>
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
	class CXXMethodDecl;
}

namespace bobopt {

	/// \brief Flushes all changes in rewriter object.
	void flush_rewriter(clang::Rewriter& rewriter);
	
	/// \brief Tests whether member function overrides bobox basic box virtual member function.
	bool overrides(clang::CXXMethodDecl* method_decl, const std::string& parent_name);

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

	template<typename NodeT>
	class ast_nodes_collector : public clang::RecursiveASTVisitor<ast_nodes_collector<NodeT>>
	{
	public:
		typedef NodeT node_type;
		typedef std::vector<NodeT*> nodes_type;
		typedef typename nodes_type::const_iterator nodes_iterator;
		typedef typename nodes_type::size_type size_type;

		nodes_iterator nodes_begin() const;
		nodes_iterator nodes_end() const;

		bool empty() const;
		size_type size() const;

		node_type* operator [](size_type index) const;

		bool VisitDecl(clang::Decl* decl);
		bool VisitStmt(clang::Stmt* stmt);
		bool VisitType(clang::Type* type);

	private:
		nodes_type nodes_;
	};

	template<typename NodeT>
	BOBOPT_INLINE typename ast_nodes_collector<NodeT>::nodes_iterator ast_nodes_collector<NodeT>::nodes_begin() const
	{
		return std::begin(nodes_);
	}

	template<typename NodeT>
	BOBOPT_INLINE typename ast_nodes_collector<NodeT>::nodes_iterator ast_nodes_collector<NodeT>::nodes_end() const
	{
		return std::end(nodes_);
	}

	template<typename NodeT>
	BOBOPT_INLINE bool ast_nodes_collector<NodeT>::empty() const
	{
		return nodes_.empty();
	}

	template<typename NodeT>
	BOBOPT_INLINE typename ast_nodes_collector<NodeT>::size_type ast_nodes_collector<NodeT>::size() const
	{
		return nodes_.size();
	}
	template<typename NodeT>
	BOBOPT_INLINE NodeT* ast_nodes_collector<NodeT>::operator [](size_type index) const
	{
		return nodes_[index];
	}

	template<typename NodeT>
	BOBOPT_INLINE bool ast_nodes_collector<NodeT>::VisitDecl(clang::Decl* decl)
	{
		BOBOPT_ASSERT(decl != nullptr);

		node_type* node = llvm::dyn_cast<node_type*>(decl);
		if (node != nullptr)
		{
			nodes_.push_back(node);
		}

		return true;
	}

	template<typename NodeT>
	BOBOPT_INLINE bool ast_nodes_collector<NodeT>::VisitStmt(clang::Stmt* stmt)
	{
		BOBOPT_ASSERT(stmt != nullptr);

		node_type* node = llvm::dyn_cast<node_type*>(stmt);
		if (node != nullptr)
		{
			nodes_.push_back(node);
		}

		return true;
	}

	template<typename NodeT>
	BOBOPT_INLINE bool ast_nodes_collector<NodeT>::VisitType(clang::Type* type)
	{
		BOBOPT_ASSERT(type != nullptr);

		node_type* node = llvm::dyn_cast<node_type*>(type);
		if (node != nullptr)
		{
			nodes_.push_back(node);
		}

		return true;
	}

} // namespace

#include BOBOPT_INLINE_IN_HEADER(bobopt_clang_utils)

#endif // guard