# This CMake module searches for Clang/LLVM
#
# Please set CMAKE_PREFIX_PATH to the folder containing Clang/LLVM source code and Clang/LLVM build directory
#
# This module sets following variables
#   clang_FOUND - If clang include directories were found.
#     clang_INCLUDE_DIRS - The list of clang include directories.
#   llvm_FOUND - If LLVM include directorios were found.
#     llvm_INCLUDE_DIRS - The list of llv include directories.
#
# Furhermore, this modules provides functions to search and add Clang/LLVM libraries to a project:
#   target_link_clang(target [clang libraries])
#   target_link_llvm(target [llvm_libraries])

#
# Headers.
#

# LLVM
set(llvm_SOURCE_HEADER llvm/Support/raw_ostream.h)
find_path(llvm_SOURCE_INCLUDE_DIRS ${llvm_SOURCE_HEADER})
if (NOT llvm_SOURCE_INCLUDE_DIRS)
	message(FATAL_ERROR "Failed to find ${llvm_SOURCE_HEADER}. Add path to CMAKE_PREFIX_PATH.")
endif ()

set(llvm_BUILD_HEADER llvm/Support/DataTypes.h)
find_path(llvm_BUILD_INCLUDE_DIRS ${llvm_BUILD_HEADER})
if (NOT llvm_BUILD_INCLUDE_DIRS)
	message(FATAL_ERROR "Failed to find ${llvm_BUILD_HEADER}. Add path to CMAKE_PREFIX_PATH.")
endif ()

if ((NOT llvm_BUILD_INCLUDE_DIRS STREQUAL llvm_SOURCE_INCLUDE_DIRS-NOTFOUND) AND (NOT llvm_BUILD_INCLUDE_DIRS STREQUAL llvm_BUILD_INCLUDE_DIRS-NOTFOUND))
	set(llvm_FOUND)
	set(llvm_INCLUDE_DIRS ${llvm_SOURCE_INCLUDE_DIRS} ${llvm_BUILD_INCLUDE_DIRS})
endif ()

# Clang
set(clang_SUFFIX_PATH tools/clang/include)
set(clang_SOURCE_HEADER ${clang_SUFFIX_PATH}/clang/Frontend/CompilerInstance.h)
find_path(clang_SOURCE_DIRECTORY ${clang_SOURCE_HEADER})
if (NOT clang_SOURCE_DIRECTORY)
	message(FATAL_ERROR "Failed to find ${clang_SOURCE_HEADER}. Add path to CMAKE_PREFIX_PATH.")
endif ()
set(clang_SOURCE_INCLUDE_DIRS ${clang_SOURCE_DIRECTORY}/${clang_SUFFIX_PATH})

set(clang_BUILD_HEADER ${clang_SUFFIX_PATH}/clang/Config/config.h)
find_path(clang_BUILD_DIRECTORY ${clang_BUILD_HEADER})
if (NOT clang_BUILD_DIRECTORY)
	message(FATAL_ERROR "Failed to find ${clang_BUILD_HEADER}. Add path to CMAKE_PREFIX_PATH.")
endif ()
set(clang_BUILD_INCLUDE_DIRS ${clang_BUILD_DIRECTORY}/${clang_SUFFIX_PATH})

if ((NOT clang_BUILD_INCLUDE_DIRS STREQUAL clang_SOURCE_INCLUDE_DIRS-NOTFOUND) AND (NOT clang_BUILD_INCLUDE_DIRS STREQUAL clang_BUILD_INCLUDE_DIRS-NOTFOUND))
	set(clang_FOUND)
	set(clang_INCLUDE_DIRS ${clang_SOURCE_INCLUDE_DIRS} ${clang_BUILD_INCLUDE_DIRS})
endif ()

#
# Libraries
#

# helpers
macro(common_link_libraries source target)
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
			find_library(${source}_${lib}_DEBUG_LIBRARY ${dir}/${fullname} NO_CMAKE_SYSTEM_PATH)
			if (${source}_${lib}_DEBUG_LIBRARY)
				break()
			endif ()
		endforeach ()

		# System based
		if (NOT ${source}_${lib}_DEBUG_LIBRARY)
			find_library(${source}_${lib}_DEBUG_LIBRARY ${fullname})
		endif ()

		#
		# Release
		#

		# CMAKE_PREFIX_PATH based
		foreach (dir ${release_DIRS})
			find_library(${source}_${lib}_RELEASE_LIBRARY ${dir}/${fullname} NO_CMAKE_SYSTEM_PATH)
			if (${source}_${lib}_RELEASE_LIBRARY)
				break()
			endif ()
		endforeach ()

		# System based
		if (NOT ${source}_${lib}_RELEASE_LIBRARY)
			find_library(${source}_${lib}_RELEASE_LIBRARY ${fullname})
		endif ()

		#
		# Finish
		#

		if ((NOT ${source}_${lib}_DEBUG_LIBRARY) AND (NOT ${source}_${lib}_RELEASE_LIBRARY))
			message(FATAL_ERROR "Clang/LLVM library ${lib} wasn't found. Add Clang/LLVM build dir to CMAKE_PREFIX_PATH.")
		endif ()

		# If there's no debug library, set debug library to release
		if (NOT ${source}_${lib}_DEBUG_LIBRARY)
			set(${source}_${lib}_DEBUG_LIBRARY ${${source}_${lib}_RELEASE_LIBRARY})
		endif ()

		# If there's no release library, set release library to debug
		if (NOT ${source}_${lib}_RELEASE_LIBRARY)
			set(${source}_${lib}_RELEASE_LIBRARY ${${source}_${lib}_DEBUG_LIBRARY})
		endif ()

		set(${source}_${lib}_LIBRARY optimized ${${source}_${lib}_RELEASE_LIBRARY} debug ${${source}_${lib}_DEBUG_LIBRARY})
		target_link_libraries(${target} ${${source}_${lib}_LIBRARY})
	endforeach ()
endmacro(target_link_llvm)

# main macros
macro(target_link_clang target)
	common_link_libraries(clang ${target} ${ARGN})
endmacro(target_link_clang)

macro(target_link_llvm target)
	common_link_libraries(llvm ${target} ${ARGN})
endmacro(target_link_llvm)