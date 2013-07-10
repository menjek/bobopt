/// \file bobopt_macros.hpp Contains definition of helpfull macros.

#ifndef BOBOPT_MACROS_HPP_GUARD_
#define BOBOPT_MACROS_HPP_GUARD_

#include <bobopt_language.hpp>

#define BOBOPT_NONCOPYABLE(class_name) \
	class_name(const class_name&) BOBOPT_SPECIAL_DELETE; \
	class_name& operator =(const class_name&) BOBOPT_SPECIAL_DELETE;

#define BOBOPT_NONMOVABLE(class_name) \
	class_name(class_name&&) BOBOPT_SPECIAL_DELETE; \
	class_name& operator =(class_name&&) BOBOPT_SPECIAL_DELETE;

#define BOBOPT_NONCOPYMOVABLE(class_name) \
	BOBOPT_NONCOPYABLE(class_name) \
	BOBOPT_NONMOVABLE(class_name)

#define BOBOPT_COUNT_OF(array_name) (sizeof((array_name)) / sizeof((array_name)[0]))

#define BOBOPT_CONCAT(lhs, rhs) (lhs ## rhs)
#define BOBOPT_CONCAT2(lhs, rhs) BOBOPT_CONCAT((lhs), (rhs))

#define BOBOPT_STRINGIFY(text) #text
#define BOBOPT_STRINGIFY2(text) BOBOPT_STRINGIFY(text)

#define BOBOPT_UNUSED_EXPRESSION(expression) (void)(expression)

#if defined(_MSC_VER)
#	define BOBOPT_PRAGMA(x) __pragma(x)
#else
#	define BOBOPT_PRAGMA(x) _Pragma(BOBOPT_STRINGIFY(x))
#endif

#endif // guard