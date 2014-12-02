/// \file bobopt_debug.hpp File contains definitions of macros for debugging.

#ifndef BOBOPT_DEBUG_HPP_GUARD_
#define BOBOPT_DEBUG_HPP_GUARD_

#include <bobopt_macros.hpp>

#include <cassert>

// BOBOPT_BREAK
//==============================================================================

/// \def BOBOPT_BREAK
/// Platform specific macro that causes runtime environment to break on code.

#ifndef NDEBUG

#if defined(_MSC_VER)
#   define BOBOPT_BREAK __debugbreak()
#elif defined(__GNUG__)
#   define BOBOPT_BREAK __builtin_trap()
#elif defined(__clang__)
#   define BOBOPT_BREAK __builtin_trap()
#else
#   define BOBOPT_BREAK
#endif

#else // NDEBUG

#   define BOBOPT_BREAK

#endif // BOBOPT_BREAK

// BOBOPT_ASSERT
//==============================================================================

/// \def BOBOPT_ASSERT
/// Macro is able to break runtime environment on specific line of code which
/// failed to pass condition.

#ifndef NDEBUG

#define BOBOPT_ASSERT(condition) assert((!!(condition)) || (BOBOPT_BREAK, false))

#if defined(_MSC_VER)
#	define BOBOPT_ASSERT_MSG(condition, text) (void)( (!!(condition)) || (BOBOPT_BREAK, _wassert(_CRT_WIDE(text), _CRT_WIDE(__FILE__), __LINE__), 0) )
#else
#	define BOBOPT_ASSERT_MSG(condition, text) BOBOPT_ASSERT(condition)
#endif

#else // NDEBUG

#	define BOBOPT_ASSERT(condition)
#	define BOBOPT_ASSERT_MSG(condition, text)

#endif // BOBOPT_ASSERT

// BOBOPT_CHECK
//==============================================================================

/// \def BOBOPT_CHECK
/// The same behaviour as \c BOBOPT_ASSERT in debug mode. In non-debug mode,
/// macro will execute code. Unlike \c BOBOPT_ASSERT which completely removes
/// code.

#ifndef NDEBUG
#	define BOBOPT_CHECK(condition) BOBOPT_ASSERT(condition)
#else // NDEBUG
#	define BOBOPT_CHECK(condition) (void)(condition)
#endif // BOBOPT_CHECK

// BOBOPT_ERROR
//==============================================================================

/// \def BOBOPT_ERROR
/// Fails on line execution with specific message.
///
/// Example of usage:
/// \code
/// switch (some_enum_type)
/// {
///     case some_enum_value1: { break; }
///     case some_enum_value2: { break; }
///     case some_enum_value3: { break; }
///     // All enum values handled.
///     default: BOBOPT_ERROR("Wrong enum value.");
/// }
/// \endcode

#ifndef NDEBUG
#	define BOBOPT_ERROR(text) BOBOPT_ASSERT_MSG(false, text)
#else // NDEBUG
#	define BOBOPT_ERROR(text)
#endif // BOBOPT_ERROR

// BOBOPT_TODO
//==============================================================================

/// \def BOBOPT_TODO
/// Macro to define TODO message which may be turned into warning on some
/// platforms.

#if defined(_MSC_VER)
#	define BOBOPT_TODO_PREFIX __FILE__ "(" BOBOPT_STRINGIFY2(__LINE__) ") : Warning: "
#else
#	define BOBOPT_TODO_PREFIX "(" __FILE__ ") " __LINE__ ": "
#endif

#ifndef NDEBUG
#	define BOBOPT_TODO(text) BOBOPT_PRAGMA(message (BOBOPT_TODO_PREFIX "TODO: " text))
#else // NDEBUG
#	define BOBOPT_TODO(text)
#endif // BOBOPT_TODO

#endif // guard