# This CMake module searches for the Bobox framework headers/libraries
#
# Please set CMAKE_PREFIX_PATH to the folder containing the Bobox framework source code
# The root directory should contain:
#   bobox
#   ulibpp
#
# This module sets variables
#   bobox_FOUND - If Bobox headers and libraries were found
#     bobox_INCLUDE_DIRS - The include directory for Bobox headers
#     bobox_LIBRARIES - The list of Bobox libraries
#   ulibpp_FOUND - If ulibpp header and libraries were found
#     ulibpp_INCLUDE_DIRS - The include directory for ulibpp headers
#     ulibpp_LIBRARIES - The list of ulibpp libraries.

# 32 vs 64
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(BUILD_PLATFORM 64)
    set(BUILD_PLATFORM_NAME "x64")
else ()
    set(BUILD_PLATFORM 32)
    set(BUILD_PLATFORM_NAME "x86")
endif ()

# Bobox
set(bobox_HEADER boback/h/bobox_basic_box.hpp)
find_path(bobox_ROOT_DIR NAMES ${bobox_HEADER})
if (NOT bobox_ROOT_DIR)
	message(FATAL_ERROR "Failed to find ${bobox_HEADER}. Add path to CMAKE_PREFIX_PATH.")
endif ()

set(bobox_INCLUDE_DIRS ${bobox_ROOT_DIR}/boback/h)

if (MSVC)
	if (BUILD_PLATFORM EQUAL 64)
		file(GLOB bobox_LIBRARY_DEBUG "${bobox_ROOT_DIR}/boback/lib/*x64.*.md.lib")
		file(GLOB bobox_LIBRARY_RELEASE "${bobox_ROOT_DIR}/boback/lib/*x64.*.mr.lib")
	else ()
		file(GLOB bobox_LIBRARY_DEBUG "${bobox_ROOT_DIR}/boback/lib/*ia32.*.md.lib")
		file(GLOB bobox_LIBRARY_RELEASE "${bobox_ROOT_DIR}/boback/lib/*ia32.*.mr.lib")
	endif ()
elseif (UNIX)
	find_file(bobox_LIBRARY_DEBUG "boback/lib/debug/boback.a")
	find_file(bobox_LIBRARY_RELEASE "boback/lib/release/boback.a")
endif ()

if ((NOT bobox_LIBRARY_DEBUG) AND (NOT bobox_LIBRARY_RELEASE))
	message(FATAL_ERROR "No Bobox library found in: ${bobox_ROOT_DIR}/boback/lib/")
endif ()

if (NOT bobox_LIBRARY_DEBUG)
	set(bobox_LIBRARY_DEBUG ${bobox_LIBRARY_RELEASE})
endif ()

if (NOT bobox_LIBRARY_RELEASE)
	set(bobox_LIBRARY_RELEASE ${bobox_LIBRARY_DEBUG})
endif ()

set(bobox_LIBRARIES optimized ${bobox_LIBRARY_RELEASE} debug ${bobox_LIBRARY_DEBUG})
set(bobox_FOUND)

# ulibpp
set(ulibpp_HEADER h/upp_fiber.hpp)
find_path(ulibpp_ROOT_DIR NAMES ${ulibpp_HEADER})
if (NOT ulibpp_ROOT_DIR)
	message(FATAL_ERROR "Failed to find ${ulibpp_HEADER}. Add path to CMAKE_PREFIX_PATH.")
endif ()

set(ulibpp_INCLUDE_DIRS ${ulibpp_ROOT_DIR}/h)

if (MSVC)
    if (BUILD_PLATFORM EQUAL 64)
        file(GLOB ulibpp_LIBRARY_DEBUG "${ulibpp_ROOT_DIR}/lib/*x64.*.md.lib")
	file(GLOB ulibpp_LIBRARY_RELEASE "${ulibpp_ROOT_DIR}/lib/*x64.*.mr.lib")
    else ()
	file(GLOB ulibpp_LIBRARY_DEBUG "${ulibpp_ROOT_DIR}/lib/*ia32.*.md.lib")
	file(GLOB ulibpp_LIBRARY_RELEASE "${ulibpp_ROOT_DIR}/lib/*ia32.*.mr.lib")
    endif ()
elseif (UNIX)
    find_file(ulibpp_LIBRARY_DEBUG "/lib/debug/ulibpp.a")
    find_file(ulibpp_LIBRARY_RELEASE "/lib/release/ulibpp.a")
endif ()

if ((NOT ulibpp_LIBRARY_DEBUG) AND (NOT ulibpp_LIBRARY_RELEASE))
	message(FATAL_ERROR "No ulibpp library found in: ${ulibpp_ROOT_DIR}/lib/")
endif ()

if (NOT ulibpp_LIBRARY_DEBUG)
	set(ulibpp_LIBRARY_DEBUG ${ulibpp_LIBRARY_RELEASE})
endif ()

if (NOT ulibpp_LIBRARY_RELEASE)
	set(ulibpp_LIBRARY_RELEASE ${ulibpp_LIBRARY_DEBUG})
endif ()

set(ulibpp_LIBRARIES optimized ${ulibpp_LIBRARY_RELEASE} debug ${ulibpp_LIBRARY_DEBUG})
set(ulibpp_FOUND)

