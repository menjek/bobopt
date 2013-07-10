#include <bobopt_optimizer.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"
#include <clang/bobopt_clang_epilog.hpp>

using namespace clang::ast_matchers;
using namespace clang::tooling;

int main(int argc, const char* argv[])
{
	CommonOptionsParser options(argc, argv);
	RefactoringTool tool(options.getCompilations(), options.getSourcePathList());

	bobopt::optimizer optimizer(&tool.getReplacements());

	MatchFinder finder;
	finder.addMatcher(bobopt::optimizer::BOX_MATCHER, &optimizer);
	
	int result = tool.runAndSave(newFrontendActionFactory(&finder));

	return result;
}
