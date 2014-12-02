/// \file bobopt_clang_prolog.hpp Prolog for any Clang/LLVM includes. It stores
/// the current warnings state and disable some of warnings.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#pragma warning(push)
#pragma warning(disable : 4100 4127 4146 4217 4244 4245 4267 4291 4345 4389 4510 4512 4610 4702 4800 4996)
