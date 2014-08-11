set(BENCHMARKS_FOLDER "benchmarks")

# 32 vs 64
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(BUILD_PLATFORM 64)
    set(BUILD_PLATFORM_NAME "x64")
else ()
    set(BUILD_PLATFORM 32)
    set(BUILD_PLATFORM_NAME "x86")
endif ()

if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${bobopt_SHARED_FLAGS}")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /DBOBOX_DEBUG /D_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
elseif (UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${bobopt_SHARED_FLAGS}")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DBOBOX_DEBUG -D_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
else ()
	message(WARNING "Unsupported platform, not setting any compilation flags.")
endif ()

# bobox
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

set(bobox_LIBRARY optimized ${bobox_LIBRARY_RELEASE} debug ${bobox_LIBRARY_DEBUG})

# ulibpp
set(ulibpp_HEADER h/upp_fiber.hpp)
find_path(ulibpp_ROOT_DIR NAMES ${ulibpp_HEADER})
if (NOT ulibpp_ROOT_DIR)
	message(FATAL_ERROR "Failed to find ${ulibpp_HEADER}. Add path to CMAKE_PREFIX_PATH.")
endif ()

set(ulibpp_INCLUDE_DIRS ${ulibpp_ROOT_DIR}/h)

if (BUILD_PLATFORM EQUAL 64)
	file(GLOB ulibpp_LIBRARY_DEBUG "${ulibpp_ROOT_DIR}/lib/*x64.*.md.lib")
	file(GLOB ulibpp_LIBRARY_RELEASE "${ulibpp_ROOT_DIR}/lib/*x64.*.mr.lib")
else ()
	file(GLOB ulibpp_LIBRARY_DEBUG "${ulibpp_ROOT_DIR}/lib/*ia32.*.md.lib")
	file(GLOB ulibpp_LIBRARY_RELEASE "${ulibpp_ROOT_DIR}/lib/*ia32.*.mr.lib")
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

set(ulibpp_LIBRARY optimized ${ulibpp_LIBRARY_RELEASE} debug ${ulibpp_LIBRARY_DEBUG})

include_directories(${bobox_INCLUDE_DIRS} ${ulibpp_INCLUDE_DIRS})

# bench utils library
set(bench_utils_SOURCES
	bobox_prolog.hpp
	bobox_epilog.hpp
	bench_utils.hpp
	bench_utils.cpp
	)
	
add_library(bench_utils ${bench_utils_SOURCES})
if (BOBOPT_FOLDERS)
	set_property(TARGET bench_utils PROPERTY FOLDER ${BENCHMARKS_FOLDER})
endif ()

macro(exclude_headers out)
	foreach (source ${ARGN})
		if (source MATCHES "(.*)\\.cpp")
			list(APPEND result ${source})
		endif ()
	endforeach()
	set(${out} ${result})
endmacro()

macro(add_optimized_program name)
	set(sources ${ARGN})
	
	# unoptimized version
	add_executable(${name} ${sources})
	target_link_libraries(${name} bench_utils ${bobox_LIBRARY} ${ulibpp_LIBRARY})
	
	# generate target for optimized build
	add_custom_target(${name}_generate)
	add_dependencies(${name}_generate ${name} bobopt)
	
	set(optimized_DIR ${CMAKE_CURRENT_BINARY_DIR}/${name}_optimized/)
	
	# copy files to new directory
	add_custom_command(TARGET ${name}_generate COMMAND ${CMAKE_COMMAND} -E make_directory ${optimized_DIR})
	
	set(optimized_sources)
	foreach (source_file ${sources})
		get_filename_component(file_name ${source_file} NAME)
		add_custom_command(TARGET ${name}_generate COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${source_file} ${optimized_DIR})
		list(APPEND optimized_sources ${optimized_DIR}/${file_name})
	endforeach ()
	
	# generate compilation database
	get_directory_property(include_dirs INCLUDE_DIRECTORIES)
	
	exclude_headers(sources_only ${optimized_sources})
	set(compilation_database ${optimized_DIR}/compile_commands.json)
	add_custom_command(TARGET ${name}_generate
		COMMAND ${CMAKE_COMMAND}
		-DINCLUDE_DIRS="${include_dirs}"
		-DOPTIMIZED_DIR="${optimized_DIR}"
		-DCOMPILATION_DATABASE="${compilation_database}"
		-DOPTIMIZED_SOURCES="${sources_only}"
		-P "${CMAKE_CURRENT_SOURCE_DIR}/gen_compile_commands.cmake"
		)
	
	# run bobopt
	get_property(bobopt_executable TARGET bobopt PROPERTY LOCATION)
	
	foreach (source ${sources_only})
		add_custom_command(TARGET ${name}_generate COMMAND ${bobopt_executable} -build ${source})
	endforeach ()
	
	# optimized build
	set_source_files_properties(${optimized_sources} PROPERTIES GENERATED true)
	add_executable(${name}_optimized ${optimized_sources})
	target_link_libraries(${name}_optimized bench_utils ${bobox_LIBRARY} ${ulibpp_LIBRARY})
	add_dependencies(${name}_optimized ${name}_generate)
	
	if (BOBOPT_FOLDERS)
		set_property(TARGET ${name} PROPERTY FOLDER ${BENCHMARKS_FOLDER})
		set_property(TARGET ${name}_generate PROPERTY FOLDER ${BENCHMARKS_FOLDER})
		set_property(TARGET ${name}_optimized PROPERTY FOLDER ${BENCHMARKS_FOLDER})
	endif ()
endmacro()

# prefetch optimization method benchmarks.
set(bobopt_benchmarks_prefetch_SOURCES
	prefetch/bench_prefetch.hpp
	prefetch/main.cpp
	)

add_optimized_program(bench_prefetch ${bobopt_benchmarks_prefetch_SOURCES})
