/// \file bobopt_inline.hpp File contains definition of inlining helper macros.

#ifndef BOBOPT_INLINE_HPP_GUARD_
#define BOBOPT_INLINE_HPP_GUARD_

#include <bobopt_macros.hpp>

/// \def BOBOPT_INLINE
/// Replacement for \c inline keyword.

/// \def BOBOPT_FORCE_INLINE
/// Macro for non-standard forced inlining.

/// \def BOBOPT_INLINE_IN_HEADER(file)
/// Macro to include inline definition file in header.

/// \def BOBOPT_INLINE_IN_SOURCE(file)
/// Macro to include inline definition file in source.

#if BOBOPT_NO_INLINE

#	define BOBOPT_INLINE
#	define BOBOPT_INLINE_IN_HEADER(file) <bobopt_empty.inl>
#	define BOBOPT_INLINE_IN_SOURCE(file) BOBOPT_STRINGIFY(file ## .inl)
#	define BOBOPT_FORCE_INLINE inline

#else // BOBOPT_NO_INLINE

#	define BOBOPT_INLINE inline
#	define BOBOPT_INLINE_IN_HEADER(file) BOBOPT_STRINGIFY(file ## .inl)
#	define BOBOPT_INLINE_IN_SOURCE(file) <bobopt_empty.inl>

#	if defined(__MSC_VER)
#		define BOBOPT_FORCE_INLINE __forceinline
#	elif defined (__GNUG__)
#		define BOBOPT_FORCE_INLINE __attribute__((always_inline))
#	elif defined (__clang__)
#		define BOBOPT_FORCE_INLINE __attribute__((always_inline))
#	else
#		define BOBOPT_FORCE_INLINE inline
#	endif // BOBOPT_FORCE_INLINE

#endif // BOBOPT_NO_INLINE

#endif // guard