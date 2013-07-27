#include <clang/bobopt_clang_utils.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/DeclCXX.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include <clang/bobopt_clang_epilog.hpp>

using namespace std;
using namespace clang;
using namespace clang::ast_matchers;

namespace bobopt {
	
	void flush_rewriter(Rewriter& rewriter)
	{
		// FIXME: copy pasted from clang source.
		for (Rewriter::buffer_iterator I = rewriter.buffer_begin(), E = rewriter.buffer_end(); I != E; ++I) {
			const FileEntry *Entry = rewriter.getSourceMgr().getFileEntryForID(I->first);
			std::string ErrorInfo;
			llvm::raw_fd_ostream FileStream(Entry->getName(), ErrorInfo, llvm::sys::fs::F_Binary);
			if (!ErrorInfo.empty())
				return;

			I->second.write(FileStream);
			FileStream.flush();
		}
	}

	bool overrides(CXXMethodDecl* method_decl, const string& parent_name)
	{
		for (auto it = method_decl->begin_overridden_methods(); it != method_decl->end_overridden_methods(); ++it)
		{
			const CXXMethodDecl* overridden = *it;
			if (overridden->getParent()->getQualifiedNameAsString() == parent_name)
			{
				return true;
			}
		}

		return false;
	}

	recursive_match_finder::recursive_match_finder(MatchFinder* match_finder, ASTContext* context)
		: match_finder_(match_finder)
		, context_(context)
	{}

	bool recursive_match_finder::VisitDecl(Decl* decl)
	{
		match_finder_->match(*decl, *context_);
		return true;
	}

	bool recursive_match_finder::VisitStmt(Stmt* stmt)
	{
		match_finder_->match(*stmt, *context_);
		return true;
	}

	bool recursive_match_finder::VisitType(Type* type)
	{
		match_finder_->match(*type, *context_);
		return true;
	}

} // namespace