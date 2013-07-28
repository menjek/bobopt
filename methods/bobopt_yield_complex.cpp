#include <methods/bobopt_yield_complex.hpp>

#include <bobopt_debug.hpp>
#include <bobopt_inline.hpp>
#include <bobopt_macros.hpp>
#include <bobopt_utils.hpp>
#include <clang/bobopt_clang_utils.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <algorithm>

using namespace std;
using namespace clang;
using namespace clang::ast_type_traits;

namespace bobopt {

	namespace methods {
		
		namespace internal {

			class code_complexity_visitor : public RecursiveASTVisitor<code_complexity_visitor>
			{
			public:
				code_complexity_visitor();

				bool VisitCallExpr(CallExpr* call_expr);

				bool call_presented() const;
				size_t get_complexity() const;

			private:
				void analyze_call(CallExpr* call_expr)
				{
					BOBOPT_ASSERT(call_expr != nullptr);
					
					FunctionDecl* callee = call_expr->getDirectCallee();
					if (callee != nullptr)
					{
						Stmt* body = callee->getBody();
						if (!analyze_compound(body))
						{
							analyze_recursive(body);
						}
					}
				}

				void analyze_recursive(Stmt* stmt)
				{
					if (stmt != nullptr)
					{
						code_complexity_visitor visitor;
						visitor.TraverseStmt(stmt);
						call_presented_ |= visitor.call_presented_;
						complexity_ += visitor.complexity_;
					}
				}

				bool analyze_compound(Stmt* stmt)
				{
					CompoundStmt* compound_stmt = llvm::dyn_cast_or_null<CompoundStmt>(stmt);
					if (compound_stmt == nullptr)
					{
						return false;
					}

					for (auto stmt_it = compound_stmt->body_begin(); stmt_it != compound_stmt->body_end(); ++stmt_it)
					{
						Stmt* stmt = *stmt_it;

						if (analyze_if(stmt) || analyze_for(stmt) || analyze_while(stmt) || analyze_switch(stmt) || analyze_try(stmt) || analyze_compound(stmt))
						{
							continue;
						}

						analyze_recursive(stmt);
					}

					return true;
				}

				bool analyze_if(Stmt* stmt)
				{
					IfStmt* if_stmt = llvm::dyn_cast_or_null<IfStmt>(stmt);
					if (if_stmt == nullptr)
					{
						return false;
					}

					Stmt* cond_stmt = if_stmt->getCond();
					code_complexity_visitor cond_visitor;
					cond_visitor.TraverseStmt(cond_stmt);

					code_complexity_visitor then_visitor;
					Stmt* then_stmt = if_stmt->getThen();
					if (then_stmt != nullptr)
					{
						then_visitor.TraverseStmt(then_stmt);
					}

					code_complexity_visitor else_visitor;
					Stmt* else_stmt = if_stmt->getElse();
					if (else_stmt != nullptr)
					{
						else_visitor.TraverseStmt(else_stmt);
					}

					call_presented_ |= cond_visitor.call_presented_ || then_visitor.call_presented_ || else_visitor.call_presented_;
					complexity_ += max(cond_visitor.complexity_, max(then_visitor.complexity_, else_visitor.complexity_));
					
					return true;
				}

				bool analyze_for(Stmt* stmt)
				{
					ForStmt* for_stmt = llvm::dyn_cast_or_null<ForStmt>(stmt);
					if (for_stmt == nullptr)
					{
						return false;
					}

					analyze_recursive(for_stmt->getInit());
					analyze_recursive(for_stmt->getCond());
					analyze_recursive(for_stmt->getInc());

					Stmt* body = for_stmt->getBody();
					if (!analyze_compound(body))
					{
						analyze_recursive(body);
					}

					return true;
				}

				bool analyze_while(Stmt* stmt)
				{
					WhileStmt* while_stmt = llvm::dyn_cast_or_null<WhileStmt>(stmt);
					if (while_stmt == nullptr)
					{
						return false;
					}

					analyze_recursive(while_stmt->getCond());

					Stmt* body = while_stmt->getBody();
					if (!analyze_compound(body))
					{
						analyze_recursive(body);
					}

					return true;
				}

				bool analyze_switch(Stmt* stmt)
				{
					SwitchStmt* switch_stmt = llvm::dyn_cast_or_null<SwitchStmt>(stmt);
					if (switch_stmt == nullptr)
					{
						return false;
					}

					analyze_recursive(switch_stmt->getCond());

					size_t complexity = 0;

					SwitchCase* switch_case = switch_stmt->getSwitchCaseList();
					while (switch_case != nullptr)
					{
						code_complexity_visitor visitor;
						visitor.TraverseStmt(switch_case->getSubStmt());
						call_presented_ |= visitor.call_presented_;
						complexity = max(complexity, visitor.complexity_);

						switch_case = switch_case->getNextSwitchCase();
					}

					complexity_ += complexity;
					return true;
				}

				bool analyze_try(Stmt* stmt)
				{
					CXXTryStmt* try_stmt = llvm::dyn_cast_or_null<CXXTryStmt>(stmt);
					if (try_stmt == nullptr)
					{
						return false;
					}

					BOBOPT_CHECK(analyze_compound(try_stmt->getTryBlock()));
					return true;
				}

				bool call_presented_;
				size_t complexity_;
			};

			BOBOPT_INLINE code_complexity_visitor::code_complexity_visitor()
				: call_presented_(false)
				, complexity_(1)
			{}

			BOBOPT_INLINE bool code_complexity_visitor::VisitCallExpr(CallExpr* call_expr)
			{
				call_presented_ = true;
				analyze_call(call_expr);
				return true;
			}

			BOBOPT_INLINE bool code_complexity_visitor::call_presented() const
			{
				return call_presented_;
			}

			BOBOPT_INLINE size_t code_complexity_visitor::get_complexity() const
			{
				return complexity_;
			}

		} // namespace internal

		yield_complex::yield_complex()
			: box_(nullptr)
			, replacements_(nullptr)
		{}

		yield_complex::~yield_complex()
		{}

		void yield_complex::optimize(clang::CXXRecordDecl* box, clang::tooling::Replacements* replacements)
		{
			BOBOPT_ASSERT(box != nullptr);
			BOBOPT_ASSERT(replacements != nullptr);

			box_ = box;
			replacements_ = replacements_;
			
			optimize_methods();
		}

		void yield_complex::optimize_methods()
		{
			BOBOPT_ASSERT(box_ != nullptr);

			for (auto method_it = box_->method_begin(); method_it != box_->method_end(); ++method_it)
			{
				CXXMethodDecl* method = *method_it;

				for (const auto& exec_method : BOX_EXEC_METHOD_OVERRIDES)
				{
					if ((method->getNameAsString() == exec_method.method_name) && overrides(method, exec_method.parent_name))
					{
						optimize_method(method);
					}
				}
			}
		}

		void yield_complex::optimize_method(exec_function_type method)
		{
			BOBOPT_ASSERT(method != nullptr);

			if (!method->hasBody())
			{
				return;
			}

			CompoundStmt* body = llvm::dyn_cast_or_null<CompoundStmt>(method->getBody());
			if (body == nullptr)
			{
				return;
			}

			complexity_tree_node_pointer root = build_complexity_tree(body);
			insert_yields(root);
		}

		size_t yield_complex::analyze_for(Stmt* init_stmt, Stmt* cond_stmt) const
		{
			BOBOPT_UNUSED_EXPRESSION(init_stmt);
			BOBOPT_UNUSED_EXPRESSION(cond_stmt);
			return FOR_UNKNOWN_LOOP_COUNT;
		}

		yield_complex::complexity_tree_node_pointer yield_complex::create_if(IfStmt* if_stmt) const
		{
			complexity_tree_node_pointer if_root = make_unique<complexity_tree_node>();

			size_t cond_complexity = 1;
			Stmt* cond_stmt = if_stmt->getCond();
			BOBOPT_ASSERT(cond_stmt != nullptr);
			complexity_tree_node_pointer cond_node = search_and_create_call(cond_stmt);
			if (cond_node)
			{
				cond_complexity = cond_node->complexity;
				if_root->children.push_back(move(cond_node));
			}

			size_t then_complexity = 0;
			Stmt* then_stmt = if_stmt->getThen();
			if (then_stmt != nullptr)
			{
				then_complexity = 1;

				CompoundStmt* then_compound = llvm::dyn_cast<CompoundStmt>(then_stmt);
				if (then_compound != nullptr)
				{
					complexity_tree_node_pointer then_node = build_complexity_tree(then_compound);
					then_complexity = then_node->complexity;
					if_root->children.push_back(move(then_node));
				}
				else
				{
					complexity_tree_node_pointer then_node = search_and_create_call(then_stmt);
					if (then_node)
					{
						then_complexity = then_node->complexity;
						if_root->children.push_back(move(then_node));
					}
				}
			}

			size_t else_complexity = 0;
			Stmt* else_stmt = if_stmt->getElse();
			if (else_stmt != nullptr)
			{
				else_complexity = 1;

				CompoundStmt* else_compound = llvm::dyn_cast<CompoundStmt>(else_stmt);
				if (else_compound != nullptr)
				{
					complexity_tree_node_pointer else_node = build_complexity_tree(else_compound);
					else_complexity = else_node->complexity;
					if_root->children.push_back(move(else_node));
				}
				else
				{
					complexity_tree_node_pointer else_node = search_and_create_call(else_stmt);
					if (else_node)
					{
						else_complexity = else_node->complexity;
						if_root->children.push_back(move(else_node));
					}
				}
			}

			if_root->node = DynTypedNode::create(*if_stmt);
			if_root->complexity = max(cond_complexity, max(then_complexity, else_complexity));
			return if_root;
		}

		yield_complex::complexity_tree_node_pointer yield_complex::create_for(ForStmt* for_stmt) const
		{
			complexity_tree_node_pointer for_node = make_unique<complexity_tree_node>();
			
			size_t body_complexity = 0;

			Stmt* body_stmt = for_stmt->getBody();
			if (body_stmt)
			{
				body_complexity = 1;

				CompoundStmt* body_compound = llvm::dyn_cast<CompoundStmt>(body_stmt);
				if (body_compound != nullptr)
				{
					complexity_tree_node_pointer body_node = build_complexity_tree(body_compound);
					body_complexity = body_node->complexity;
					for_node->children.push_back(move(body_node));
				}
			}

			for_node->complexity = body_complexity;

			size_t loops = analyze_for(for_stmt->getInit(), for_stmt->getCond());
			if(loops != FOR_UNKNOWN_LOOP_COUNT)
			{
				for_node->complexity *= loops;
			}

			for_node->node = DynTypedNode::create(*for_stmt);
			return for_node;
		}

		yield_complex::complexity_tree_node_pointer yield_complex::create_while(WhileStmt* while_stmt) const
		{
			complexity_tree_node_pointer while_node = make_unique<complexity_tree_node>();

			size_t body_complexity = 1;

			Stmt* body_stmt = while_stmt->getBody();
			if (body_stmt != nullptr)
			{
				CompoundStmt* body_compound = llvm::dyn_cast<CompoundStmt>(body_stmt);
				if (body_compound != nullptr)
				{
					complexity_tree_node_pointer body_node = build_complexity_tree(body_compound);
					body_complexity = body_node->complexity;
					while_node->children.push_back(move(body_node));
				}
			}

			while_node->node = DynTypedNode::create(*while_stmt);
			while_node->complexity = body_complexity;
			return while_node;
		}

		yield_complex::complexity_tree_node_pointer yield_complex::create_switch(SwitchStmt* switch_stmt) const
		{
			complexity_tree_node_pointer switch_node = make_unique<complexity_tree_node>();

			size_t switch_complexity = 1;

			SwitchCase* switch_case = switch_stmt->getSwitchCaseList();
			while (switch_case != nullptr)
			{
				CompoundStmt* switch_case_compound = llvm::dyn_cast_or_null<CompoundStmt>(switch_case->getSubStmt());
				if (switch_case_compound != nullptr)
				{
					complexity_tree_node_pointer switch_case_node = build_complexity_tree(switch_case_compound);
					switch_complexity = max(switch_complexity, switch_case_node->complexity);
					switch_node->children.push_back(move(switch_case_node));
				}

				switch_case = switch_case->getNextSwitchCase();
			}

			switch_node->node = DynTypedNode::create(*switch_stmt);
			switch_node->complexity = switch_complexity;
			return switch_node;
		}

		yield_complex::complexity_tree_node_pointer yield_complex::create_try(CXXTryStmt* try_stmt) const
		{
			complexity_tree_node_pointer try_node = make_unique<complexity_tree_node>();

			size_t block_complexity = 1;

			CompoundStmt* block_compound = llvm::dyn_cast_or_null<CompoundStmt>(try_stmt->getTryBlock());
			if (block_compound != nullptr)
			{
				complexity_tree_node_pointer block_node = build_complexity_tree(block_compound);
				block_complexity = block_node->complexity;
				try_node->children.push_back(move(block_node));
			}

			try_node->node = DynTypedNode::create(*try_stmt);
			try_node->complexity = block_complexity;
			return try_node;
		}

		yield_complex::complexity_tree_node_pointer yield_complex::search_and_create_call(Stmt* stmt) const
		{
			internal::code_complexity_visitor visitor;
			visitor.TraverseStmt(stmt);
			if (visitor.call_presented())
			{
				complexity_tree_node_pointer stmt_node = make_unique<complexity_tree_node>();
				stmt_node->node = DynTypedNode::create(*stmt);
				stmt_node->complexity = visitor.get_complexity();
				return stmt_node;
			}

			return nullptr;
		}

		yield_complex::complexity_tree_node_pointer yield_complex::build_complexity_tree(clang::CompoundStmt* compound_stmt) const
		{
			unique_ptr<complexity_tree_node> root = make_unique<complexity_tree_node>();

			size_t complexity = 0;
			for (auto stmt_it = compound_stmt->body_begin(); stmt_it != compound_stmt->body_end(); ++stmt_it)
			{
				Stmt* stmt = *stmt_it;

				IfStmt* if_stmt = llvm::dyn_cast<IfStmt>(stmt);
				if (if_stmt != nullptr)
				{
					complexity_tree_node_pointer if_node = create_if(if_stmt);
					complexity += if_node->complexity;
					root->children.push_back(move(if_node));
					continue;
				}

				ForStmt* for_stmt = llvm::dyn_cast<ForStmt>(stmt);
				if (for_stmt != nullptr)
				{
					complexity_tree_node_pointer for_node = create_for(for_stmt);
					complexity += for_node->complexity;
					root->children.push_back(move(for_node));
					continue;
				}

				WhileStmt* while_stmt = llvm::dyn_cast<WhileStmt>(stmt);
				if (while_stmt != nullptr)
				{
					complexity_tree_node_pointer while_node = create_while(while_stmt);
					complexity += while_node->complexity;
					root->children.push_back(move(while_node));
					continue;
				}

				SwitchStmt* switch_stmt = llvm::dyn_cast<SwitchStmt>(stmt);
				if (switch_stmt != nullptr)
				{
					complexity_tree_node_pointer switch_node = create_switch(switch_stmt);
					complexity += switch_node->complexity;
					root->children.push_back(move(switch_node));
					continue;
				}

				CXXTryStmt* try_stmt = llvm::dyn_cast<CXXTryStmt>(stmt);
				if (try_stmt != nullptr)
				{
					complexity_tree_node_pointer try_node = create_try(try_stmt);
					complexity += try_node->complexity;
					root->children.push_back(move(try_node));
					continue;
				}

				complexity_tree_node_pointer stmt_node = search_and_create_call(stmt);
				if (stmt_node)
				{
					complexity += stmt_node->complexity;
					root->children.push_back(move(stmt_node));
					continue;
				}

				++complexity;
			}

			root->node = DynTypedNode::create(*compound_stmt);
			root->complexity = complexity;
			return root;
		}

		void yield_complex::insert_yields(const complexity_tree_node_pointer& root) const
		{
			BOBOPT_UNUSED_EXPRESSION(root);
		}

		// constants:
		const yield_complex::method_override yield_complex::BOX_EXEC_METHOD_OVERRIDES[BOX_EXEC_METHOD_COUNT] =
		{
			{ {"sync_mach_etwas"}, {"bobox::basic_box"} },
			{ {"async_mach_etwas"}, {"bobox::basic_box"} },
			{ {"body_mach_etwas"}, {"bobox::basic_box"} }
		};

	} // namespace

	basic_method* create_yield_complex()
	{
		return new methods::yield_complex;
	}

} // namespace