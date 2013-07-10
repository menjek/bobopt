/// \file bobopt_language.hpp Contains definition of macros dependent on language support.

#ifndef BOBOPT_LANGUAGE_HPP_GUARD_
#define BOBOPT_LANGUAGE_HPP_GUARD_

// constexpr specifier.
//==============================================================================

// vs
#if defined(_MSC_VER)
#	define BOBOPT_CONSTEXPR

// gcc
#elif defined(__GNUG__)
#	if ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 6)) || (__GNUC__ > 4)
#		define BOBOPT_CONSTEXPR constexpr
#	else
#		define BOBOPT_CONSTEXPR
#	endif

// clang
#elif defined(__clang__)
#	if (__has_feature(cxx_constexpr))
#		define BOBOPT_CONSTEXPR constexpr
#	else
#		define BOBOPT_CONSTEXPR
#	endif

// other
#else
#	define BOBOPT_CONSTEXPR
#endif // BOBOPT_CONSTEXPR

// override specifier.
//==============================================================================

// vs
#if defined(_MSC_VER)
#	define BOBOPT_OVERRIDE

// gcc
#elif defined(__GNUG__)
#	define BOBOPT_OVERRIDE override

// clang
#elif defined(__clang__)
#	define BOBOPT_OVERRIDE override
#endif

// final specifier.
//==============================================================================

// vs
#if defined(_MSC_VER)
#	define BOBOPT_FINAL

// gcc
#elif defined(__GNUG__)
#	define BOBOPT_FINAL final

// clang
#elif defined(__clang__)
#	define BOBOPT_FINAL final
#endif


// defaulted and deleted functions.
//==============================================================================

// vs
#if defined(_MSC_VER)
#	define BOBOPT_SPECIAL_DEFAULT
#	define BOBOPT_SPECIAL_DELETE

// gcc
#elif defined(__GNUG__)
#	if ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 4)) || (__GNUC__ > 4)
#		define BOBOPT_SPECIAL_DEFAULT = default
#		define BOBOPT_SPECIAL_DELETE = delete
#	else
#		define BOBOPT_SPECIAL_DEFAULT
#		define BOBOPT_SPECIAL_DELETE
#	endif

// clang
#elif defined(__clang__)
#	if (__has_feature(cxx_defaulted_functions))
#		define BOBOPT_SPECIAL_DEFAULT = default
#	else
#		define BOBOPT_SPECIAL_DEFAULT
#	endif

#	if  (__has_feature(cxx_deleted_functions))
#		define BOBOPT_SPECIAL_DELETE = delete
#	else
#		define BOBOPT_SPECIAL_DELETE
#	endif

// other
#else
#	define BOBOPT_SPECIAL_DEFAULT
#	define BOBOPT_SPECIAL_DELETE
#endif // BOBOPT_SPECIAL_DEFAULT/BOBOPT_SPECIAL_DELETE


#endif // guard