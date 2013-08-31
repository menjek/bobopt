#include <bobopt_debug.hpp>
#include <bobopt_inline.hpp>

#include <iterator>

namespace bobopt {

	namespace methods {

		// complexity inline implementation.
		//==============================================================================

		BOBOPT_INLINE void complexity::add(complexity_type amount)
		{
			min_complexity_ += amount;
			avg_complexity_ += amount;
			max_complexity_ += amount;

			if (use_heuristic_)
			{
				heuristic_complexity_ += amount;
			}
		}

		BOBOPT_INLINE void complexity::add(const complexity& other)
		{
			min_complexity_ += other.min_complexity_;
			avg_complexity_ += other.avg_complexity_;
			max_complexity_ += other.max_complexity_;

			use_heuristic_ &= use_heuristic_;
			heuristic_complexity_ += other.heuristic_complexity_;
		}

		BOBOPT_INLINE clang::Stmt* complexity::get_ast_stmt() const
		{
			return ast_stmt_;
		}

		BOBOPT_INLINE complexity::complexity_type complexity::get_min_complexity() const
		{
			return min_complexity_;
		}

		BOBOPT_INLINE complexity::complexity_type complexity::get_avg_complexity() const
		{
			return avg_complexity_;
		}

		BOBOPT_INLINE complexity::complexity_type complexity::get_max_complexity() const
		{
			return max_complexity_;
		}

		BOBOPT_INLINE bool complexity::is_heuristic() const
		{
			return use_heuristic_;
		}

		BOBOPT_INLINE complexity::complexity_type complexity::get_heuristic_complexity() const
		{
			return heuristic_complexity_;
		}

		// compound_complexity inline implementation.
		//==============================================================================

		BOBOPT_INLINE compound_complexity::children_iterator compound_complexity::children_begin() const
		{
			return std::begin(children_);
		}

		BOBOPT_INLINE compound_complexity::children_iterator compound_complexity::children_end() const
		{
			return std::end(children_);
		}

		// branch_complexity inline implementation.
		//==============================================================================

		BOBOPT_INLINE branch_complexity::branches_iterator branch_complexity::branches_begin() const
		{
			return std::begin(branches_);
		}

		BOBOPT_INLINE branch_complexity::branches_iterator branch_complexity::branches_end() const
		{
			return std::end(branches_);
		}

		BOBOPT_INLINE branch_complexity::size_type branch_complexity::get_branch_count() const
		{
			return branches_.size();
		}

		BOBOPT_INLINE complexity* branch_complexity::get_branch(size_type index) const
		{
			BOBOPT_ASSERT(index < get_branch_count());
			return branches_[index].get();
		}

		// if_complexity inline implementation.
		//==============================================================================

		BOBOPT_INLINE complexity* if_complexity::get_then() const
		{
			if (then_index_ == nindex)
			{
				return nullptr;
			}

			BOBOPT_ASSERT(then_index_ < get_branch_count());
			return get_branch(then_index_);
		}

		BOBOPT_INLINE complexity* if_complexity::get_else() const
		{
			if (else_index_ == nindex)
			{
				return nullptr;
			}

			BOBOPT_ASSERT(else_index_ < get_branch_count());
			return get_branch(else_index_);
		}

		// switch_complexity inline implementation.
		//==============================================================================

		BOBOPT_INLINE complexity* switch_complexity::get_case(size_type index) const
		{
			BOBOPT_ASSERT(index < get_cases_count());
			BOBOPT_ASSERT(index < get_branch_count());

			return get_branch(indices_[index]);
		}

		BOBOPT_INLINE switch_complexity::size_type switch_complexity::get_cases_count() const
		{
			BOBOPT_ASSERT(indices_.size() <= get_branch_count());

			return indices_.size();
		}

		// loop_complexity inline implementation.
		//==============================================================================

		BOBOPT_INLINE loop_complexity::loop_kind loop_complexity::get_kind() const
		{
			return kind_;
		}

		BOBOPT_INLINE complexity* loop_complexity::get_body() const
		{
			return body_.get();
		}

		BOBOPT_INLINE size_t loop_complexity::get_min_loop_count() const
		{
			return min_loop_;
		}

		BOBOPT_INLINE size_t loop_complexity::get_avg_loop_count() const
		{
			return avg_loop_;
		}

		BOBOPT_INLINE size_t loop_complexity::get_max_loop_count() const
		{
			return max_loop_;
		}

	} // namespace

} // namespace