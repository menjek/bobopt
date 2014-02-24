project(bobopt)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_SUPPRESS_REGENERATION true)

if (MSVC)
  add_definitions(/W4)
  add_definitions(/wd4068)
  add_definitions(/D "_SCL_SECURE_NO_WARNINGS")
endif ()

if (UNIX)
  add_definitions(-Wall -Wextra -Wno-unknown-pragmas)
  add_definitions(-std=c++0x -fno-rtti)
  add_definitions(-D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS)
endif ()

# Find Clang/LLVM headers + libraries.
# It's up to CMake user to setup basic path to project (e.g using CMAKE_PREFIX_PATH)

#
# Headers.
#

# LLVM
set(llvm_HEADER llvm/Support/raw_ostream.h)
find_path(llvm_INCLUDE_DIRS ${llvm_HEADER})
if (NOT llvm_INCLUDE_DIRS)
  message(FATAL_ERROR "Failed to find ${llvm_HEADER}. Add path to CMAKE_PREFIX_PATH.")
endif ()

set(llvm_BUILD_HEADER llvm/Support/DataTypes.h)
find_path(llvm_BUILD_INCLUDE_DIRS ${llvm_BUILD_HEADER})
if (NOT llvm_BUILD_INCLUDE_DIRS)
  message(FATAL_ERROR "Failed to find ${llvm_BUILD_HEADER}. Add path to CMAKE_PREFIX_PATH.")
endif ()

# Clang
set(clang_SUFFIX_PATH tools/clang/include)
set(clang_HEADER ${clang_SUFFIX_PATH}/clang/Frontend/CompilerInstance.h)
find_path(clang_INCLUDE_DIRS ${clang_HEADER})
if (NOT clang_INCLUDE_DIRS)
  message(FATAL_ERROR "Failed to find ${clang_HEADER}. Add path to CMAKE_PREFIX_PATH.")
endif ()
set(clang_INCLUDE_DIRS ${clang_INCLUDE_DIRS}/${clang_SUFFIX_PATH})

set(clang_BUILD_HEADER ${clang_SUFFIX_PATH}/clang/Config/config.h)
find_path(clang_BUILD_INCLUDE_DIRS ${clang_BUILD_HEADER})
if (NOT clang_BUILD_INCLUDE_DIRS)
  message(FATAL_ERROR "Failed to find ${clang_BUILD_HEADER}. Add path to CMAKE_PREFIX_PATH.")
endif ()
set(clang_BUILD_INCLUDE_DIRS ${clang_BUILD_INCLUDE_DIRS}/${clang_SUFFIX_PATH})

#
# Libraries
#

# Helpers
macro(find_libraries_impl libs)
  if (MSVC)
    set(prefix "")
    set(extension ".lib")

    set(debug_DIRS "Debug/lib")
    set(release_DIRS "Release/lib")
  endif ()

  if (UNIX)
    set(prefix "lib")
    set(extension ".a")

    set(debug_DIRS "Debug+Asserts/lib")
    set(release_DIRS "Release+Asserts/lib")
  endif ()

  foreach (lib ${ARGN})
    set(fullname ${prefix}${lib}${extension})

    find_library(clangllvm_${lib}_DEBUG_LIBRARY ${debug_DIRS}/${fullname} NO_CMAKE_SYSTEM_PATH)
    find_library(clangllvm_${lib}_RELEASE_LIBRARY ${release_DIRS}/${fullname} NO_CMAKE_SYSTEM_PATH)
   
    if ((NOT clangllvm_${lib}_DEBUG_LIBRARY) AND (NOT clangllvm_${lib}_RELEASE_LIBRARY))
      message(FATAL_ERROR "Clang/LLVM library ${lib} wasn't found. Add Clang/LLVM build dir to CMAKE_PREFIX_PATH.")
    endif ()
   
    # If there's no debug library, set debug library to release
    if (NOT clangllvm_${lib}_DEBUG_LIBRARY)
      set(clangllvm_${lib}_DEBUG_LIBRARY ${clangllvm_${lib}_RELEASE_LIBRARY})
    endif ()
    
    # If there's no release library, set release library to debug
    if (NOT clangllvm_${lib}_RELEASE_LIBRARY)
      set(clangllvm_${lib}_RELEASE_LIBRARY ${clangllvm_${lib}_DEBUG_LIBRARY})
    endif ()

    set(clangllvm_${lib}_LIBRARY optimized ${clangllvm_${lib}_RELEASE_LIBRARY} debug ${clangllvm_${lib}_DEBUG_LIBRARY})
    list(APPEND ${libs} ${clangllvm_${lib}_LIBRARY})
  endforeach ()
endmacro(find_libraries_impl)

macro(find_libraries libs)
  find_libraries_impl(${libs} ${ARGN})
endmacro(find_libraries)

# Clang/LLVM
find_libraries(clangllvm_LIBRARIES
  clangTooling
  clangFrontendTool
  clangFrontend
  clangDriver
  clangSerialization
  clangParse
  clangSema
  clangStaticAnalyzerFrontend
  clangStaticAnalyzerCore
  clangRewriteFrontend
  clangRewriteCore
  clangAnalysis
  clangEdit
  clangAST
  clangASTMatchers
  clangLex
  clangBasic
  LLVMBitWriter
  LLVMOption
  LLVMMCParser
  LLVMTransformUtils
  LLVMBitReader
  LLVMMC
  LLVMCore
  LLVMSupport
  )

# Setup sources.
set(bobopt_SOURCES
  clang/bobopt_clang_utils.cpp
  clang/bobopt_clang_epilog.hpp
  clang/bobopt_clang_prolog.hpp
  clang/bobopt_clang_utils.hpp
  clang/bobopt_control_flow_search.hpp
  clang/bobopt_clang_utils.inl
  methods/bobopt_prefetch.cpp
  methods/bobopt_yield_complex.cpp
  methods/bobopt_prefetch.hpp
  methods/bobopt_yield_complex.hpp
  bobopt_config.cpp
  bobopt_diagnostic.cpp
  bobopt_method.cpp
  bobopt_method_factory.cpp
  bobopt_optimizer.cpp
  bobopt_text_utils.cpp
  main.cpp
  bobopt_config.hpp
  bobopt_debug.hpp
  bobopt_diagnostic.hpp
  bobopt_inline.hpp
  bobopt_language.hpp
  bobopt_macros.hpp
  bobopt_method.hpp
  bobopt_method_factory.hpp
  bobopt_optimizer.hpp
  bobopt_parser.hpp
  bobopt_text_utils.hpp
  bobopt_utils.hpp
  bobopt_diagnostic.inl
  bobopt_empty.inl
  bobopt_method.inl
  bobopt_optimizer.inl
  bobopt_text_utils.inl
  )
 
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${llvm_INCLUDE_DIRS} ${llvm_BUILD_INCLUDE_DIRS})
include_directories(${clang_INCLUDE_DIRS} ${clang_BUILD_INCLUDE_DIRS})
add_executable(bobopt ${bobopt_SOURCES})

if (UNIX)
  set(clangllvm_CUSTOM_LIBRARIES -lz -lpthread -lffi -lcurses -ldl -lm)
endif ()

target_link_libraries(bobopt ${clangllvm_CUSTOM_LIBRARIES} ${clangllvm_LIBRARIES})
