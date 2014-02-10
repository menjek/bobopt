/// \file bobopt_macros.hpp File contains definition of miscellaneous macros.

#ifndef BOBOPT_MACROS_HPP_GUARD_
#define BOBOPT_MACROS_HPP_GUARD_

#include <bobopt_language.hpp>

// Copy/Move protection.
//==============================================================================

/// \def BOBOPT_NONCOPYABLE(class_name)
/// Macro needs to be defined in \c private section of class in C++98.
/// It supresses copy constructor and copy assignment operator.

#define BOBOPT_NONCOPYABLE(class_name) \
	class_name(const class_name&) BOBOPT_SPECIAL_DELETE; \
	class_name& operator =(const class_name&) BOBOPT_SPECIAL_DELETE;

/// \def BOBOPT_NONMOVABLE(class_name)
/// Macro needs to be defined in \c private section of class in C++98.
/// It supresses move constructor and move assignment operator.

#define BOBOPT_NONMOVABLE(class_name) \
	class_name(class_name&&) BOBOPT_SPECIAL_DELETE; \
	class_name& operator =(class_name&&) BOBOPT_SPECIAL_DELETE;

/// \def BOBOPT_NONCOPYMOVABLE(class_name)
/// Macro needs to be defined in \c private section of class in C++98.
/// It combines \ref BOBOPT_NONCOPYABLE(class_name) and
/// \ref BOBOPT_NONMOVABLE(class_name).

#define BOBOPT_NONCOPYMOVABLE(class_name) \
	BOBOPT_NONCOPYABLE(class_name) \
	BOBOPT_NONMOVABLE(class_name)

// BOBOPT_COUNT_OF
//==============================================================================

/// \def BOBOPT_COUNT_OF(array_name)
/// Number of elements in C-like array.

#define BOBOPT_COUNT_OF(array_name) (sizeof((array_name)) / sizeof((array_name)[0]))

// Preprocessor operations.
//==============================================================================

#define BOBOPT_CONCAT(lhs, rhs) (lhs ## rhs)
#define BOBOPT_CONCAT2(lhs, rhs) BOBOPT_CONCAT((lhs), (rhs))

#define BOBOPT_STRINGIFY(text) #text
#define BOBOPT_STRINGIFY2(text) BOBOPT_STRINGIFY(text)

// BOBOPT_UNUSED_EXPRESSION
//==============================================================================

/// \def BOBOPT_UNUSED_EXPRESSION(expression)
/// Macro supresses compiler warnings about unused parameters and other
/// variables. (Local variables for RAII).

#define BOBOPT_UNUSED_EXPRESSION(expression) (void)(expression)

// BOBOPT_PRAGMA
//==============================================================================

/// \def BOBOPT_PRAGMA(x)
/// Macro wraps platform specific pragmas.

#if defined(_MSC_VER)
#	define BOBOPT_PRAGMA(x) __pragma(x)
#else
#	define BOBOPT_PRAGMA(x) _Pragma(BOBOPT_STRINGIFY(x))
#endif

#endif // guard