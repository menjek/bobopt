/// \file bobopt_debug.hpp File contains basic macros for debugging.

#ifndef BOBOPT_DEBUG_HPP_GUARD_
#define BOBOPT_DEBUG_HPP_GUARD_

#include <bobopt_macros.hpp>

#include <cassert>

// BOBOPT_BREAK
//==============================================================================

#ifndef NDEBUG

#if defined(_MSC_VER)
#   define BOBOPT_BREAK __debugbreak()
#elif defined(__GNUG__)
#   define BOBOPT_BREAK __builtin_trap()
#elif defined(__clang__)
#   define BOBOPT_BREAK __builtin_trap()
#else
#   define BOBOPT_BREAK ((void)0)
#endif

#else // NDEBUG

#   define BOBOPT_BREAK

#endif // BOBOPT_BREAK

// BOBOPT_ASSERT
//==============================================================================

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

#ifndef NDEBUG
#	define BOBOPT_CHECK(condition) BOBOPT_ASSERT(condition)
#else // NDEBUG
#	define BOBOPT_CHECK(condition) (void)(condition)
#endif // BOBOPT_CHECK

// BOBOPT_ERROR
//==============================================================================

#ifndef NDEBUG
#	define BOBOPT_ERROR(text) BOBOPT_ASSERT_MSG(false, text)
#else // NDEBUG
#	define BOBOPT_ERROR(text)
#endif // BOBOPT_ERROR

// BOBOPT_TODO
//==============================================================================

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