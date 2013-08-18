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

		BOBOPT_INLINE clang::ast_type_traits::DynTypedNode complexity::get_ast_node() const
		{
			return ast_node_;
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

	} // namespace

} // namespace