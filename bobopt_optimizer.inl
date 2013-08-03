#include <bobopt_debug.hpp>

#include <algorithm>
#include <iterator>
#include <type_traits>

namespace bobopt {

	template<typename InputIterator>
	optimizer::optimizer(clang::tooling::Replacements* replacements, InputIterator first, InputIterator last)
		: replacements_(replacements)
	{
		construct(first, last);
	}

	template<typename InputIterator>
	void optimizer::construct(InputIterator first, InputIterator last)
	{
		std::fill_n(std::begin(methods_), methods_.size(), nullptr);
		for (; first != last; ++first)
		{
			create_method(*first);
		}
	}

	BOBOPT_INLINE void optimizer::enable_method(method_type method)
	{
		create_method(method);
	}

	BOBOPT_INLINE void optimizer::disable_method(method_type method)
	{
		destroy_method(method);
	}

	BOBOPT_INLINE bool optimizer::is_method_enabled(method_type method) const
	{
		BOBOPT_ASSERT(method < OM_COUNT);
		return (methods_[method] != nullptr);
	}

	BOBOPT_INLINE void optimizer::set_compiler(clang::CompilerInstance* compiler)
	{
		compiler_ = compiler;
	}

	BOBOPT_INLINE clang::CompilerInstance* optimizer::get_compiler() const
	{
		return compiler_;
	}

} // namespace