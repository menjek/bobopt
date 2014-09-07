project(bobopt)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/make/modules)

set(CMAKE_SUPPRESS_REGENERATION true)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (MSVC)
	set(bobopt_SHARED_FLAGS "/W4 /wd4068 /D_SCL_SECURE_NO_WARNINGS /EHsc")
	set(bobopt_CXX_FLAGS "${bobopt_SHARED_FLAGS}")
endif ()

if (UNIX)
	set(bobopt_SHARED_FLAGS "-Wall -Wextra -Wno-unknown-pragmas -std=c++0x")
	set(bobopt_CXX_FLAGS "${bobopt_SHARED_FLAGS} -fno-rtti -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS")
endif ()

# Clang
find_package(Clang REQUIRED)

set(clang_LIBRARIES
	clangTooling
	clangFrontendTool
	clangFrontend
	clangDriver
	clangSerialization
	clangParse
	clangSema
	clangStaticAnalyzerFrontend
	clangStaticAnalyzerCore
	clangRewrite
	clangRewriteFrontend
	clangAnalysis
	clangEdit
	clangAST
	clangASTMatchers
	clangLex
	clangBasic
	)
  
set(llvm_LIBRARIES
	LLVMBitWriter
	LLVMOption
	LLVMMCParser
	LLVMTransformUtils
	LLVMBitReader
	LLVMMC
	LLVMCore
	LLVMSupport
	)

# Sources
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
	bobopt_config.inl
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
include_directories(${llvm_INCLUDE_DIRS})
include_directories(${clang_INCLUDE_DIRS})

if (UNIX)
	set(llvm_SYSTEM_LIBRARIES -lz -lpthread -lffi -lcurses -ldl -lm)
endif ()

# Targets
add_library(bobopt_core ${bobopt_root_SOURCES} ${bobopt_clang_SOURCES})
set_target_properties(bobopt_core PROPERTIES COMPILE_FLAGS ${bobopt_CXX_FLAGS})

add_library(bobopt_methods ${bobopt_methods_SOURCES})
set_target_properties(bobopt_methods PROPERTIES COMPILE_FLAGS ${bobopt_CXX_FLAGS})

add_executable(bobopt main.cpp)
set_target_properties(bobopt PROPERTIES COMPILE_FLAGS ${bobopt_CXX_FLAGS})
target_link_libraries(bobopt bobopt_core bobopt_methods ${llvm_SYSTEM_LIBRARIES})
target_link_llvm(bobopt ${llvm_LIBRARIES})
target_link_clang(bobopt ${clang_LIBRARIES})

if (BOBOPT_FOLDERS)
	source_group("clang" FILES ${bobopt_clang_SOURCES})
	source_group("" FILES ${bobopt_root_SOURCES})
endif ()

# Benchmarks

set(BUILD_BENCHMARKS true CACHE BOOL "Build unit tests for optimization methods.")

if (BUILD_BENCHMARKS)
	add_subdirectory(benchmarks)
endif ()
