#include <bobopt_method.hpp>

#include <bobopt_inline.hpp>

#include BOBOPT_INLINE_IN_SOURCE(bobopt_method)

namespace bobopt {

	basic_method::basic_method()
		: compiler_(nullptr)
	{}

	basic_method::~basic_method()
	{}

} // namespace