/// \file bobopt_complexity_tree.hpp File contains definitions of complexity tree nodes
/// for yield_complex optimization method.

#ifndef BOBOPT_METHODS_BOBOPT_COMPLEXITY_TREE_HPP_GUARD_
#define BOBOPT_METHODS_BOBOPT_COMPLEXITY_TREE_HPP_GUARD_

#include <bobopt_inline.hpp>
#include <bobopt_macros.hpp>

#include <memory>
#include <vector>

// forward declarations:
namespace clang {
	class CallExpr;
	class CompoundStmt;
	class IfStmt;
	class Stmt;
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

			clang::Stmt* get_ast_stmt() const;

			complexity_type get_min_complexity() const;
			complexity_type get_avg_complexity() const;
			complexity_type get_max_complexity() const;

			bool is_heuristic() const;
			complexity_type get_heuristic_complexity() const;

			static const complexity_type ncomplexity;

		protected:
			complexity();

			clang::Stmt* ast_stmt_;
			
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

			virtual ~compound_complexity();
			
			children_iterator children_begin() const;
			children_iterator children_end() const;
		
		private:
			compound_complexity();

			children_type children_;
		};

		class branch_complexity : public complexity
		{
		private:
			typedef std::vector<complexity_ptr> branches_type;

		public:
			typedef branches_type::size_type size_type;
			typedef branches_type::const_iterator branches_iterator;

			virtual ~branch_complexity();
			
			branches_iterator branches_begin() const;
			branches_iterator branches_end() const;

			size_type get_branch_count() const;
			complexity* get_branch(size_type index) const;

			static const size_type nindex;
			
		protected:
			branch_complexity();

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
			if_complexity();

			size_type then_index_;
			size_type else_index_;
		};

		class switch_complexity : public branch_complexity
		{
		public:
			static complexity_ptr create(clang::SwitchStmt* switch_stmt);

			virtual ~switch_complexity();

			complexity* get_case(size_type index) const;
			size_type get_cases_count() const;

		private:
			switch_complexity();

			typedef std::vector<size_type> indices_type;
			indices_type indices_;
		};

		class loop_complexity : public complexity
		{
		public:
			enum loop_kind
			{
				kind_for,
				kind_while,
				kind_do
			};

			static complexity_ptr create(clang::ForStmt* for_stmt);
			static complexity_ptr create(clang::WhileStmt* while_stmt);
			static complexity_ptr create(clang::DoStmt* do_stmt);
			static void create_loop_body(loop_complexity& node, clang::Stmt* body_stmt);

			virtual ~loop_complexity();

			loop_kind get_kind() const;
			complexity* get_body() const;

			size_t get_min_loop_count() const;
			size_t get_avg_loop_count() const;
			size_t get_max_loop_count() const;

		private:
			explicit loop_complexity(loop_kind kind);

			loop_kind kind_;

			complexity_ptr body_;
			
			size_t min_loop_;
			size_t avg_loop_;
			size_t max_loop_;
		};

	} // namespace methods

} // namespace bobopt

#include BOBOPT_INLINE_IN_HEADER(bobopt_complexity_tree)

#endif // BOBOPT_METHODS_BOBOPT_COMPLEXITY_TREE_HPP_GUARD_