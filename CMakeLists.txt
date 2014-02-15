project(bobopt)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_SUPPRESS_REGENERATION true)

if (MSVC)
  add_definitions(/W4)
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

set(clang_BUILD_HEADER ${clang_SUFFIX_PATH}//clang/Config/config.h)
find_path(clang_BUILD_INCLUDE_DIRS ${clang_BUILD_HEADER})
if (NOT clang_BUILD_INCLUDE_DIRS)
  message(FATAL_ERROR "Failed to find ${clang_BUILD_HEADER}. Add path to CMAKE_PREFIX_PATH.")
endif ()
set(clang_BUILD_INCLUDE_DIRS ${clang_BUILD_INCLUDE_DIRS}/${clang_SUFFIX_PATH})

#
# Libraries
#

# Helpers
macro(find_libraries_impl path libs)
  if (MSVC)
    set(extension ".lib")
  endif ()

  foreach (lib ${ARGN})
    find_library(clangllvm_${lib}_DEBUG_LIBRARY ${path}/debug/${lib}${extension})
    find_library(clangllvm_${lib}_RELEASE_LIBRARY ${path}/release/${lib}${extension})
   
    if ((NOT clangllvm_${lib}_DEBUG_LIBRARY) AND (NOT clangllvm_${lib}_RELEASE_LIBRARY))
      message(FATAL_ERROR "Clang/LLVM library ${lib} wasn't found. Add Clang/LLVM build/lib dir to CMAKE_PREFIX_PATH.")
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
  find_libraries_impl("lib" ${libs} ${ARGN})
endmacro(find_libraries)

# Clang/LLVM
find_libraries(clangllvm_LIBRARIES
  clangAnalysis
  clangARCMigrate
  clangAST
  clangASTMatchers
  clangBasic
  clangCodeGen
  clangDriver
  clangDynamicASTMatchers
  clangEdit
  clangFormat
  clangFrontend
  clangFrontendTool
  clangLex
  clangParse
  clangRewriteCore
  clangRewriteFrontend
  clangSema
  clangSerialization
  clangStaticAnalyzerCore
  clangStaticAnalyzerFrontend
  clangStaticAnalyzerCheckers
  clangTooling
  libclang
  llvm_headers_do_not_build
  LLVMAArch64AsmParser
  LLVMAArch64AsmPrinter
  LLVMAArch64CodeGen
  LLVMAArch64Desc
  LLVMAArch64Disassembler
  LLVMAArch64Info
  LLVMAArch64Utils
  LLVMAnalysis
  LLVMARMAsmParser
  LLVMARMAsmPrinter
  LLVMARMCodeGen
  LLVMARMDesc
  LLVMARMDisassembler
  LLVMARMInfo
  LLVMAsmParser
  LLVMAsmPrinter
  LLVMBitReader
  LLVMBitWriter
  LLVMCodeGen
  LLVMCore
  LLVMCppBackendCodeGen
  LLVMCppBackendInfo
  LLVMDebugInfo
  LLVMExecutionEngine
  LLVMHexagonAsmPrinter
  LLVMHexagonCodeGen
  LLVMHexagonDesc
  LLVMHexagonInfo
  LLVMInstCombine
  LLVMInstrumentation
  LLVMInterpreter
  LLVMipa
  LLVMipo
  LLVMIRReader
  LLVMJIT
  LLVMLinker
  LLVMMC
  LLVMMCDisassembler
  LLVMMCJIT
  LLVMMCParser
  LLVMMipsAsmParser
  LLVMMipsAsmPrinter
  LLVMMipsCodeGen
  LLVMMipsDesc
  LLVMMipsDisassembler
  LLVMMipsInfo
  LLVMMSP430AsmPrinter
  LLVMMSP430CodeGen
  LLVMMSP430Desc
  LLVMMSP430Info
  LLVMNVPTXAsmPrinter
  LLVMNVPTXCodeGen
  LLVMNVPTXDesc
  LLVMNVPTXInfo
  LLVMObjCARCOpts
  LLVMObject
  LLVMOption
  LLVMPowerPCAsmParser
  LLVMPowerPCAsmPrinter
  LLVMPowerPCCodeGen
  LLVMPowerPCDesc
  LLVMPowerPCInfo
  LLVMR600AsmPrinter
  LLVMR600CodeGen
  LLVMR600Desc
  LLVMR600Info
  LLVMRuntimeDyld
  LLVMScalarOpts
  LLVMSelectionDAG
  LLVMSparcCodeGen
  LLVMSparcDesc
  LLVMSparcInfo
  LLVMSupport
  LLVMSystemZAsmParser
  LLVMSystemZAsmPrinter
  LLVMSystemZCodeGen
  LLVMSystemZDesc
  LLVMSystemZDisassembler
  LLVMSystemZInfo
  LLVMTableGen
  LLVMTarget
  LLVMTransformUtils
  LLVMVectorize
  LLVMX86AsmParser
  LLVMX86AsmPrinter
  LLVMX86CodeGen
  LLVMX86Desc
  LLVMX86Disassembler
  LLVMX86Info
  LLVMX86Utils
  LLVMXCoreAsmPrinter
  LLVMXCoreCodeGen
  LLVMXCoreDesc
  LLVMXCoreDisassembler
  LLVMXCoreInfo
  profile_rt
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
target_link_libraries(bobopt ${clangllvm_LIBRARIES})