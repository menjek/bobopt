#include <bobopt_debug.hpp>
#include <bobopt_inline.hpp>
#include <bobopt_utils.hpp>
#include <clang/bobopt_clang_utils.hpp>
#include <methods/bobopt_complexity_tree.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/Stmt.h"
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
			: min_complexity_(0)
			, avg_complexity_(0)
			, max_complexity_(0)
			, use_heuristic_(false)
			, heuristic_complexity_(0)
		{}

		complexity::~complexity()
		{}

		// compound_complexity implementation.
		//==============================================================================

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

					for (auto call_expr_it = collector.nodes_begin(); call_expr_it != collector.nodes_end(); ++call_expr_it)
					{
						result->add(call_expr_complexity(*call_expr_it));
					}
				}
			}

			return complexity_ptr(result);
		}

		// branch_complexity implementation.
		//==============================================================================

		branch_complexity::~branch_complexity()
		{}

		const branch_complexity::size_type branch_complexity::nindex = numeric_limits<branch_complexity::size_type>::max();

		if_complexity::~if_complexity()
		{}

		complexity_ptr if_complexity::create(IfStmt* if_stmt)
		{
			BOBOPT_ASSERT(if_stmt != nullptr);

			if_complexity* result = new if_complexity();

			// condition.
			Expr* cond_expr = if_stmt->getCond();
			BOBOPT_ASSERT(cond_expr != nullptr);
			
			ast_nodes_collector<CallExpr> collector;
			collector.TraverseStmt(cond_expr);
			if (!collector.empty())
			{
				for (auto call_expr_it = collector.nodes_begin(); call_expr_it != collector.nodes_end(); ++call_expr_it)
				{
					result->add(call_expr_complexity(*call_expr_it));
				}
			}
			else
			{
				result->add(1);
			}

			// then.
			complexity_ptr then_complexity;
			Stmt* then_stmt = if_stmt->getThen();
			if (then_stmt != nullptr)
			{
				then_complexity = create_complexity_node(then_stmt);
			}

			// else.
			complexity_ptr else_complexity;
			Stmt* else_stmt = if_stmt->getElse();
			if (else_stmt != nullptr)
			{
				else_complexity = create_complexity_node(else_stmt);
			}

			result->then_index_ = nindex;
			result->else_index_ = nindex;

			BOBOPT_ASSERT(result->branches_.empty());
			if (then_complexity)
			{
				result->branches_.push_back(move(then_complexity));
				result->then_index_ = 0;
			}

			if (else_complexity)
			{
				result->branches_.push_back(move(else_complexity));
				if (result->then_index_ == 0)
				{
					result->else_index_ = 1;
				}
				else
				{
					result->else_index_ = 0;
				}
			}

			return complexity_ptr(result);
		}

		switch_complexity::~switch_complexity()
		{}

		complexity_ptr switch_complexity::create(SwitchStmt* switch_stmt)
		{
			BOBOPT_ASSERT(switch_stmt != nullptr);

			switch_complexity* result = new switch_complexity();

			// condition is considered as 1 until more complicated complexity algorithm comes in.
			result->add(1);
			result->use_heuristic_ = false;


			return complexity_ptr(result);
		}

		loop_complexity::~loop_complexity()
		{}

		complexity_ptr loop_complexity::create(ForStmt* for_stmt)
		{
			return nullptr;
		}

		complexity_ptr loop_complexity::create(WhileStmt* while_stmt)
		{
			return nullptr;
		}

		complexity_ptr loop_complexity::create(DoStmt* do_stmt)
		{
			return nullptr;
		}


	} // namespace

} // namespace
