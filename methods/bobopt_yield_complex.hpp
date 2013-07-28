#ifndef BOBOPT_METHODS_BOBOPT_YIELD_COMPLEX_HPP_GUARD_
#define BOBOPT_METHODS_BOBOPT_YIELD_COMPLEX_HPP_GUARD_

#include <bobopt_method.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/ASTTypeTraits.h"
#include "clang/Tooling/Refactoring.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <memory>
#include <string>
#include <vector>

// forward declarations:
namespace clang {
	class CXXRecordDecl;
	class CXXMethodDecl;
	class CompoundStmt;
	class Stmt;
	class IfStmt;
	class ForStmt;
	class WhileStmt;
	class CompoundStmt;
	class CXXTryStmt;
}

namespace bobopt {

	namespace methods {

		class yield_complex : public basic_method
		{
		public:
		
			// create/destroy:
			yield_complex();
			virtual ~yield_complex() BOBOPT_OVERRIDE;

			// optimize:
			virtual void optimize(clang::CXXRecordDecl* box, clang::tooling::Replacements* replacements) BOBOPT_OVERRIDE;

		private:
			
			// helpers:
			struct method_override
			{
				std::string method_name;
				std::string parent_name;
			};

			struct complexity_tree_node;
			typedef std::unique_ptr<complexity_tree_node> complexity_tree_node_pointer;

			struct complexity_tree_node
			{
				clang::ast_type_traits::DynTypedNode node;
				size_t complexity;
				std::vector<complexity_tree_node_pointer> children;
			};

			// typedefs:
			typedef clang::CXXMethodDecl* exec_function_type;

			// helpers:
			void optimize_methods();
			void optimize_method(exec_function_type method);

			size_t analyze_for(clang::Stmt* init_stmt, clang::Stmt* cond_stmt) const;

			complexity_tree_node_pointer create_if(clang::IfStmt* if_stmt) const;
			complexity_tree_node_pointer create_for(clang::ForStmt* for_stmt) const;
			complexity_tree_node_pointer create_while(clang::WhileStmt* while_stmt) const;
			complexity_tree_node_pointer create_switch(clang::SwitchStmt* switch_stmt) const;
			complexity_tree_node_pointer create_try(clang::CXXTryStmt* try_stmt) const;

			complexity_tree_node_pointer search_and_create_call(clang::Stmt* stmt) const;

			complexity_tree_node_pointer build_complexity_tree(clang::CompoundStmt* compound_stmt) const;
			void insert_yields(const complexity_tree_node_pointer& root) const;

			// data members:
			clang::CXXRecordDecl* box_;
			clang::tooling::Replacements* replacements_;

			// constants.
			static const size_t FOR_UNKNOWN_LOOP_COUNT = static_cast<size_t>(-1);
			static const size_t FOR_RUNTIME_ANALYSIS_TRESHOLD = 50;
			static const size_t BOX_EXEC_METHOD_COUNT = 3;
			static const method_override BOX_EXEC_METHOD_OVERRIDES[BOX_EXEC_METHOD_COUNT];
		};

	} // namespace methods

	basic_method* create_yield_complex();

} // namespace bobopt

#endif // guard