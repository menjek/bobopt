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
macro(find_libraries libs target)
  if (MSVC)
    set(prefix "")
    set(extension ".lib")
  endif ()

  if (UNIX)
    set(prefix "lib")
    set(extension ".a")    
  endif ()
  
  set(debug_DIRS "Debug+Asserts/lib" "Debug/lib")
  set(release_DIRS "Release+Asserts/lib" "Release/lib")

  foreach (lib ${ARGN})
    set(fullname ${prefix}${lib}${extension})

    #
    # Debug
    #
    
    # CMAKE_PREFIX_PATH based
    foreach (dir ${debug_DIRS})
      find_library(${target}_${lib}_DEBUG_LIBRARY ${dir}/${fullname} NO_CMAKE_SYSTEM_PATH)
      if (${target}_${lib}_DEBUG_LIBRARY)
        break()
      endif ()
    endforeach ()
    
    # System based
    if (NOT ${target}_${lib}_DEBUG_LIBRARY)
      find_library(${target}_${lib}_DEBUG_LIBRARY ${fullname})
    endif ()
    
    #
    # Release
    #
    
    # CMAKE_PREFIX_PATH based
    foreach (dir ${release_DIRS})
      find_library(${target}_${lib}_RELEASE_LIBRARY ${dir}/${fullname} NO_CMAKE_SYSTEM_PATH)
      if (${target}_${lib}_RELEASE_LIBRARY)
        break()
      endif ()
    endforeach ()
   
    # System based
    if (NOT ${target}_${lib}_RELEASE_LIBRARY)
      find_library(${target}_${lib}_RELEASE_LIBRARY ${fullname})
    endif ()
   
    #
    # Finish
    #
   
    if ((NOT ${target}_${lib}_DEBUG_LIBRARY) AND (NOT ${target}_${lib}_RELEASE_LIBRARY))
      message(FATAL_ERROR "Clang/LLVM library ${lib} wasn't found. Add Clang/LLVM build dir to CMAKE_PREFIX_PATH.")
    endif ()
   
    # If there's no debug library, set debug library to release
    if (NOT ${target}_${lib}_DEBUG_LIBRARY)
      set(${target}_${lib}_DEBUG_LIBRARY ${${target}_${lib}_RELEASE_LIBRARY})
    endif ()
    
    # If there's no release library, set release library to debug
    if (NOT ${target}_${lib}_RELEASE_LIBRARY)
      set(${target}_${lib}_RELEASE_LIBRARY ${${target}_${lib}_DEBUG_LIBRARY})
    endif ()

    set(${target}_${lib}_LIBRARY optimized ${${target}_${lib}_RELEASE_LIBRARY} debug ${${target}_${lib}_DEBUG_LIBRARY})
    list(APPEND ${libs} ${${target}_${lib}_LIBRARY})
  endforeach ()
endmacro(find_libraries)

# Clang/LLVM
find_libraries(clang_LIBRARIES clang
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
  )
  
find_libraries(llvm_LIBRARIES llvm
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
set(bobopt_clang_SOURCES
  clang/bobopt_clang_utils.cpp
  clang/bobopt_clang_epilog.hpp
  clang/bobopt_clang_prolog.hpp
  clang/bobopt_clang_utils.hpp
  clang/bobopt_control_flow_search.hpp
  clang/bobopt_clang_utils.inl
  )
  
set(bobopt_methods_SOURCES
  methods/bobopt_prefetch.cpp
  methods/bobopt_yield_complex.cpp
  methods/bobopt_prefetch.hpp
  methods/bobopt_yield_complex.hpp
  )
  
set(bobopt_root_SOURCES
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
  
set(bobopt_SOURCES
  ${bobopt_clang_SOURCES}
  ${bobopt_methods_SOURCES}
  ${bobopt_root_SOURCES}
  )

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${llvm_INCLUDE_DIRS} ${llvm_BUILD_INCLUDE_DIRS})
include_directories(${clang_INCLUDE_DIRS} ${clang_BUILD_INCLUDE_DIRS})
add_executable(bobopt ${bobopt_SOURCES})

if (BOBOPT_FOLDERS)
  source_group("clang" FILES ${bobopt_clang_SOURCES})
  source_group("methods" FILES ${bobopt_methods_SOURCES})
  source_group("" FILES ${bobopt_root_SOURCES})
endif ()

if (UNIX)
  set(llvm_SYSTEM_LIBRARIES -lz -lpthread -lffi -lcurses -ldl -lm)
endif ()

target_link_libraries(bobopt ${llvm_SYSTEM_LIBRARIES} ${clang_LIBRARIES} ${llvm_LIBRARIES})
