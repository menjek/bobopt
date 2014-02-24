/// \file bobopt_clang_prolog.hpp Prolog for any clang or llvm inludes. It stores warnings state and disable some of them.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

#pragma clang diagnostic push

#pragma warning(push)
#pragma warning(disable : 4100 4127 4146 4217 4244 4245 4267 4291 4345 4510 4512 4610 4702 4800 4996)
