/// \file bobopt_complexity_tree.hpp File contains definitions of complexity tree nodes
/// for yield_complex optimization method.

#ifndef BOBOPT_METHODS_BOBOPT_COMPLEXITY_TREE_HPP_GUARD_
#define BOBOPT_METHODS_BOBOPT_COMPLEXITY_TREE_HPP_GUARD_

#include <bobopt_inline.hpp>
#include <bobopt_macros.hpp>

#include <clang/bobopt_clang_prolog.hpp>
#include "clang/AST/ASTTypeTraits.h"
#include <clang/bobopt_clang_epilog.hpp>

#include <memory>
#include <vector>

// forward declarations:
namespace clang {
	class CallExpr;
	class CompoundStmt;
	class IfStmt;
	class SwitchStmt;
	class WhileStmt;
	class ForStmt;
	class DoStmt;
}

namespace bobopt {

	namespace methods {
		
		class complexity
		{
		public:
			typedef std::unique_ptr<complexity> complexity_ptr;
			typedef size_t complexity_type;
		
			virtual ~complexity();

			void add(complexity_type amount);
			void add(const complexity& other);

			clang::ast_type_traits::DynTypedNode get_ast_node() const;

			complexity_type get_min_complexity() const;
			complexity_type get_avg_complexity() const;
			complexity_type get_max_complexity() const;

			bool is_heuristic() const;
			complexity_type get_heuristic_complexity() const;

			static const complexity_type ncomplexity;

		protected:
			complexity();

			clang::ast_type_traits::DynTypedNode ast_node_;
			
			complexity_type min_complexity_;
			complexity_type avg_complexity_;
			complexity_type max_complexity_;

			bool use_heuristic_;
			complexity_type heuristic_complexity_;						

		private:
			BOBOPT_NONCOPYMOVABLE(complexity);
		};

		typedef complexity::complexity_ptr complexity_ptr;

		class compound_complexity : public complexity
		{
		private:
			typedef std::vector<complexity_ptr> children_type;

		public:
			typedef children_type::const_iterator children_iterator;

			static complexity_ptr create(clang::CompoundStmt* compound_stmt);
			
			children_iterator children_begin() const;
			children_iterator children_end() const;
		
		private:
			children_type children_;
		};

		class branch_complexity : public complexity
		{
		private:
			typedef std::vector<complexity_ptr> branches_type;

		public:
			typedef branches_type::size_type size_type;
			typedef branches_type::iterator branches_iterator;

			virtual ~branch_complexity();
			
			branches_iterator branches_begin() const;
			branches_iterator branches_end() const;

			size_type get_branch_count() const;
			complexity* get_branch(size_type index) const;

			static const size_type nindex;
			
		protected:
			branches_type branches_;
		};

		class if_complexity : public branch_complexity
		{
		public:
			static complexity_ptr create(clang::IfStmt* if_stmt);

			virtual ~if_complexity();

			complexity* get_then() const;
			complexity* get_else() const;

		private:
			size_type then_index_;
			size_type else_index_;
		};

		class switch_complexity : public branch_complexity
		{
		public:
			static complexity_ptr create(clang::SwitchStmt* switch_stmt);

			virtual ~switch_complexity();

			complexity* get_case() const;
			size_type get_cases_count() const;

		private:
			typedef std::vector<size_type> indices_type;
			indices_type indices_;
		};

		class loop_complexity : public complexity
		{
		public:
			static complexity_ptr create(clang::ForStmt* for_stmt);
			static complexity_ptr create(clang::WhileStmt* while_stmt);
			static complexity_ptr create(clang::DoStmt* do_stmt);

			virtual ~loop_complexity();

			complexity* get_body() const;
			complexity* get_cond() const;

			size_t get_min_loop_count() const;
			size_t get_avg_loop_count() const;
			size_t get_max_loop_count() const;

		private:
			complexity_ptr body_;
			complexity_ptr cond_;
			
			size_t min_loop_;
			size_t avg_loop_;
			size_t max_loop_;
		};

	} // namespace methods

} // namespace bobopt

#include BOBOPT_INLINE_IN_HEADER(bobopt_complexity_tree)

#endif // BOBOPT_METHODS_BOBOPT_COMPLEXITY_TREE_HPP_GUARD_