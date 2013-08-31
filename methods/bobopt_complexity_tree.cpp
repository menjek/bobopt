#include <bobopt_debug.hpp>
#include <bobopt_inline.hpp>
#include <bobopt_macros.hpp>
#include <bobopt_utils.hpp>
#include <clang/bobopt_clang_utils.hpp>
#include <methods/bobopt_complexity_tree.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "llvm/Support/Casting.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <algorithm>
#include <limits>
#include <utility>

using namespace std;
using namespace clang;

#include BOBOPT_INLINE_IN_SOURCE(bobopt_complexity_tree)

namespace bobopt {
	
	namespace methods {
		
		namespace {

			static const complexity::complexity_type call_boost = 200;
			static const complexity::complexity_type call_inline_boost = 10;
			static const complexity::complexity_type call_trivial_boost = 30;

			complexity_ptr create_complexity_node(Stmt* stmt)
			{
				BOBOPT_ASSERT(stmt != nullptr);

				CompoundStmt* compound_stmt = llvm::dyn_cast<CompoundStmt>(stmt);
				if (compound_stmt != nullptr)
				{
					return compound_complexity::create(compound_stmt);
				}
				
				IfStmt* if_stmt = llvm::dyn_cast<IfStmt>(stmt);
				if (if_stmt != nullptr)
				{
					return if_complexity::create(if_stmt);
				}

				ForStmt* for_stmt = llvm::dyn_cast<ForStmt>(stmt);
				if (for_stmt != nullptr)
				{
					return loop_complexity::create(for_stmt);
				}

				WhileStmt* while_stmt = llvm::dyn_cast<WhileStmt>(stmt);
				if (while_stmt != nullptr)
				{
					return loop_complexity::create(while_stmt);
				}

				SwitchStmt* switch_stmt = llvm::dyn_cast<SwitchStmt>(stmt);
				if (switch_stmt != nullptr)
				{
					return switch_complexity::create(switch_stmt);
				}

				DoStmt* do_stmt = llvm::dyn_cast<DoStmt>(stmt);
				if (do_stmt != nullptr)
				{
					return loop_complexity::create(do_stmt);
				}

				CXXTryStmt* try_stmt = llvm::dyn_cast<CXXTryStmt>(stmt);
				if (try_stmt != nullptr)
				{
					CompoundStmt* try_block = try_stmt->getTryBlock();
					BOBOPT_ASSERT(try_block != nullptr);
					return compound_complexity::create(try_block);
				}

				return nullptr;
			}

			size_t call_expr_complexity(CallExpr* call_expr)
			{
				FunctionDecl* callee = call_expr->getDirectCallee();
				if (callee != nullptr)
				{
					if (callee->isInlined())
					{
						return call_inline_boost;
					}

					if (callee->isTrivial())
					{
						return call_trivial_boost;
					}
				}

				return call_boost;
			}

		} // namespace


		// complexity implementation.
		//==============================================================================

		const complexity::complexity_type complexity::ncomplexity = std::numeric_limits<complexity::complexity_type>::max();

		complexity::complexity()
			: ast_stmt_(nullptr)
			, min_complexity_(0)
			, avg_complexity_(0)
			, max_complexity_(0)
			, use_heuristic_(false)
			, heuristic_complexity_(0)
		{}

		complexity::~complexity()
		{}

		// compound_complexity implementation.
		//==============================================================================

		compound_complexity::compound_complexity()
			: children_()
		{}

		compound_complexity::~compound_complexity()
		{}

		complexity_ptr compound_complexity::create(CompoundStmt* compound_stmt)
		{
			BOBOPT_ASSERT(compound_stmt != nullptr);

			compound_complexity* result = new compound_complexity();
			for (Stmt** child = compound_stmt->body_begin(); child != compound_stmt->body_end(); ++child)
			{
				complexity_ptr child_complexity = create_complexity_node(*child);
				if (child_complexity)
				{
					result->add(*child_complexity);
					result->children_.push_back(move(child_complexity));
				}
				else
				{
					ast_nodes_collector<CallExpr> collector;
					collector.TraverseStmt(*child);
					if (collector.empty())
					{
						result->add(1);
						continue;
					}

					auto it = collector.nodes_begin();
					for (; it != collector.nodes_end(); ++it)
					{
						result->add(call_expr_complexity(*it));
					}
				}
			}

			result->ast_stmt_ = compound_stmt;
			return complexity_ptr(result);
		}

		// branch_complexity implementation.
		//==============================================================================

		branch_complexity::branch_complexity()
			: branches_()
		{}

		branch_complexity::~branch_complexity()
		{}

		const branch_complexity::size_type branch_complexity::nindex = numeric_limits<branch_complexity::size_type>::max();

		// if_complexity implementation.
		//==============================================================================

		if_complexity::if_complexity()
			: then_index_(nindex)
			, else_index_(nindex)
		{}

		if_complexity::~if_complexity()
		{}

		complexity_ptr if_complexity::create(IfStmt* if_stmt)
		{
			BOBOPT_ASSERT(if_stmt != nullptr);

			if_complexity* result = new if_complexity();

			// Condition has to be presented.
			// It also is as "simple" as its complexity depends only on CallExpr nodes.
			Expr* cond_expr = if_stmt->getCond();
			BOBOPT_ASSERT(cond_expr != nullptr);
			
			ast_nodes_collector<CallExpr> cond_collector;
			cond_collector.TraverseStmt(cond_expr);
			if (!cond_collector.empty())
			{
				auto it = cond_collector.nodes_begin();
				for (; it != cond_collector.nodes_end(); ++it)
				{
					result->add(call_expr_complexity(*it));
				}
			}
			else
			{
				result->add(1);
			}

			// Handle complexity of then branch.
			complexity_ptr then_complexity;
			Stmt* then_stmt = if_stmt->getThen();
			if (then_stmt != nullptr)
			{
				then_complexity = create_complexity_node(then_stmt);

				// Then is statement of uknown type.
				// Search for CallExpr to evaluate its complexity.
				if (!then_complexity)
				{
					ast_nodes_collector<CallExpr> then_collector;
					then_collector.TraverseStmt(then_stmt);
					if (!then_collector.empty())
					{
						auto it = then_collector.nodes_begin();
						for (; it != then_collector.nodes_end(); ++it)
						{
							result->add(call_expr_complexity(*it));
						}
					}
					else
					{
						result->add(1);
					}
				}
			}

			// Handle complexity of else branch.
			complexity_ptr else_complexity;
			Stmt* else_stmt = if_stmt->getElse();
			if (else_stmt != nullptr)
			{
				else_complexity = create_complexity_node(else_stmt);

				// Else is statement of uknown type.
				// Search for CallExpr to evaluate its complexity.
				ast_nodes_collector<CallExpr> else_collector;
				else_collector.TraverseStmt(else_stmt);
				if (!else_collector.empty())
				{
					auto it = else_collector.nodes_begin();
					for (; it != else_collector.nodes_end(); ++it)
					{
						result->add(call_expr_complexity(*it));
					}
				}
				else
				{
					result->add(1);
				}
			}

			// Calculate complexities in branching.
			complexity_type min_complexity = numeric_limits<complexity_type>::max();
			complexity_type avg_complexity = 0;
			complexity_type max_complexity = numeric_limits<complexity_type>::min();

			if (then_complexity)
			{
				if (else_complexity)
				{
					min_complexity = min(then_complexity->get_min_complexity(), else_complexity->get_min_complexity());
					avg_complexity = (then_complexity->get_avg_complexity() + else_complexity->get_avg_complexity()) / 2;
					max_complexity = max(then_complexity->get_max_complexity(), else_complexity->get_max_complexity());
				}
				else
				{
					min_complexity = 0;
					avg_complexity = then_complexity->get_avg_complexity() / 2;
					max_complexity = then_complexity->get_max_complexity();
				}
			}
			else
			{
				if (else_complexity)
				{
					min_complexity = 0;
					avg_complexity = else_complexity->get_avg_complexity() / 2;
					max_complexity = else_complexity->get_max_complexity();
				}
			}

			result->min_complexity_ += min_complexity;
			result->avg_complexity_ += avg_complexity;
			result->max_complexity_ += max_complexity;

			// Store complexity nodes.
			result->then_index_ = nindex;
			result->else_index_ = nindex;

			BOBOPT_ASSERT(result->branches_.empty());
			if (then_complexity)
			{
				result->then_index_ = 0;
				result->branches_.push_back(move(then_complexity));
			}

			if (else_complexity)
			{
				result->else_index_ = (result->then_index_ == 0) ? 1 : 0;
				result->branches_.push_back(move(else_complexity));
			}

			result->ast_stmt_ = if_stmt;
			return complexity_ptr(result);
		}

		// switch_complexity implementation.
		//==============================================================================

		switch_complexity::switch_complexity()
			: indices_()
		{}

		switch_complexity::~switch_complexity()
		{}

		complexity_ptr switch_complexity::create(SwitchStmt* switch_stmt)
		{
			BOBOPT_ASSERT(switch_stmt != nullptr);

			switch_complexity* result = new switch_complexity();

			// Condition has to be presented.
			// It has to be as "simple" as its complexity depends only on CallExpr nodes.
			Expr* cond_expr = switch_stmt->getCond();
			BOBOPT_ASSERT(cond_expr != nullptr);
			
			ast_nodes_collector<CallExpr> cond_collector;
			cond_collector.TraverseStmt(cond_expr);
			if (!cond_collector.empty())
			{
				auto it = cond_collector.nodes_begin();
				for (; it != cond_collector.nodes_end(); ++it)
				{
					result->add(call_expr_complexity(*it));
				}
			}
			else
			{
				result->add(1);
			}

			// For each case create own node, if it is meaningful.
			// Case of type "case CONSTANT: variable = value; break;" doesn't need any own node.

			complexity_type min_complexity = numeric_limits<complexity_type>::max();
			complexity_type avg_sum_complexity = 0;
			complexity_type max_complexity = numeric_limits<complexity_type>::min();
			size_t case_count = 0;

			SwitchCase* switch_case = switch_stmt->getSwitchCaseList();
			for (; switch_case != nullptr; switch_case = switch_case->getNextSwitchCase(), ++case_count)
			{
				Stmt* sub_stmt = switch_case->getSubStmt();
				if (sub_stmt != nullptr)
				{
					complexity_ptr case_node = create_complexity_node(sub_stmt);
					if (case_node)
					{
						min_complexity = min(min_complexity, case_node->get_min_complexity());
						avg_sum_complexity += case_node->get_avg_complexity();
						max_complexity = max(max_complexity, case_node->get_max_complexity());

						result->branches_.push_back(move(case_node));
						result->indices_.push_back(result->branches_.size() - 1);
					}
					else
					{
						complexity_type case_complexity = 0;

						// Uknown case statement, search for CallExpr nodes.
						ast_nodes_collector<CallExpr> case_collector;
						case_collector.TraverseStmt(sub_stmt);
						if (!case_collector.empty())
						{
							auto it = case_collector.nodes_begin();
							for (; it != case_collector.nodes_end(); ++it)
							{
								case_complexity += call_expr_complexity(*it);
							}
						}
						else
						{
							case_complexity = 1;
						}

						min_complexity = min(result->min_complexity_, case_complexity);
						avg_sum_complexity += case_complexity;
						max_complexity = max(result->max_complexity_, case_complexity);
					}
				}
			}

			result->min_complexity_ = min_complexity;
			result->avg_complexity_ = avg_sum_complexity / case_count;
			result->max_complexity_ = max_complexity;
			result->ast_stmt_ = switch_stmt;
			return complexity_ptr(result);
		}

		// loop_complexity implementation.
		//==============================================================================

		loop_complexity::loop_complexity(loop_kind kind)
			: kind_(kind)
			, body_()
			, min_loop_(0)
			, avg_loop_(0)
			, max_loop_(0)
		{}

		loop_complexity::~loop_complexity()
		{}

		complexity_ptr loop_complexity::create(ForStmt* for_stmt)
		{
			BOBOPT_ASSERT(for_stmt != nullptr);

			loop_complexity* result = new loop_complexity(kind_for);

			// Assume init statement is "simple" in the way it can contain only CallExpr nodes.
			// Init statement is often not presented.
			Stmt* init_stmt = for_stmt->getInit();
			if (init_stmt != nullptr)
			{
				ast_nodes_collector<CallExpr> collector;
				collector.TraverseStmt(init_stmt);
				if (!collector.empty())
				{
					auto it = collector.nodes_begin();
					for (; it != collector.nodes_end(); ++it)
					{
						result->add(call_expr_complexity(*it));
					}
				}
			}

			// Do not try to analyze condition and increase.
			// It would be overheaded. We assume both have linear complexity (programmers practices).
			result->add(2);

			// Forward body creation to common loop body creation function.
			Stmt* body_stmt = for_stmt->getBody();
			if (body_stmt != nullptr)
			{
				create_loop_body(*result, body_stmt);
			}

			result->ast_stmt_ = for_stmt;
			return complexity_ptr(result);
		}

		complexity_ptr loop_complexity::create(WhileStmt* while_stmt)
		{
			BOBOPT_ASSERT(while_stmt != nullptr);

			loop_complexity* result = new loop_complexity(kind_while);

			// Condition should be presented and its complexity should depend only on CallExpr nodes.
			Expr* cond_expr = while_stmt->getCond();
			BOBOPT_ASSERT(cond_expr != nullptr);

			ast_nodes_collector<CallExpr> cond_collector;
			cond_collector.TraverseStmt(cond_expr);
			if (!cond_collector.empty())
			{
				auto it = cond_collector.nodes_begin();
				for (; it != cond_collector.nodes_end(); ++it)
				{
					result->add(call_expr_complexity(*it));
				}
			}
			else
			{
				// No CallExpr means it has the simplest complexity.
				result->add(1);
			}

			// Forward body creation to common loop body creation function.
			Stmt* body_stmt = while_stmt->getBody();
			if (body_stmt != nullptr)
			{
				create_loop_body(*result, body_stmt);
			}

			result->ast_stmt_ = while_stmt;
			return complexity_ptr(result);
		}

		complexity_ptr loop_complexity::create(DoStmt* do_stmt)
		{
			BOBOPT_ASSERT(do_stmt != nullptr);

			loop_complexity* result = new loop_complexity(kind_do);

			// Condition should be presented and its complexity should depend only on CallExpr nodes.
			Expr* cond_expr = do_stmt->getCond();
			BOBOPT_ASSERT(cond_expr != nullptr);

			ast_nodes_collector<CallExpr> cond_collector;
			cond_collector.TraverseStmt(cond_expr);
			if (!cond_collector.empty())
			{
				auto it = cond_collector.nodes_begin();
				for (; it != cond_collector.nodes_end(); ++it)
				{
					result->add(call_expr_complexity(*it));
				}
			}
			else
			{
				// No CallExpr means it has the simplest complexity.
				result->add(1);
			}

			// Forward body creation to common loop body creation function.
			Stmt* body_stmt = do_stmt->getBody();
			if (body_stmt != nullptr)
			{
				create_loop_body(*result, body_stmt);
			}

			result->ast_stmt_ = do_stmt;
			return complexity_ptr(result);
		}

		void loop_complexity::create_loop_body(loop_complexity& node, Stmt* body_stmt)
		{
			BOBOPT_ASSERT(body_stmt != nullptr);

			// Body creates its own complexity tree node as it can be any complex.
			node.body_ = create_complexity_node(body_stmt);
			if (node.body_)
			{
				node.add(*node.body_);
			}
			else
			{
				// Handle uknown complexity node type looking for CallExpr.
				ast_nodes_collector<CallExpr> body_collector;
				body_collector.TraverseStmt(body_stmt);
				if (body_collector.empty())
				{
					auto it = body_collector.nodes_begin();
					for (; it != body_collector.nodes_end(); ++it)
					{
						node.add(call_expr_complexity(*it));
					}
				}
				else
				{
					// Body is there but unknown type of statement without calls.
					node.add(1);
				}
			}
		}

	} // namespace

} // namespace
