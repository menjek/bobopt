set(BENCHMARKS_FOLDER "benchmarks")

if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${bobopt_SHARED_FLAGS}")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /DBOBOX_DEBUG /D_DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
elseif (UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${bobopt_SHARED_FLAGS}")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D BOBOX_DEBUG -D _DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread -lnuma -lrt")
else ()
	message(WARNING "Unsupported platform, not setting any compilation flags.")
endif ()

# bobox
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../make/modules)

find_package(Bobox REQUIRED)
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

set(clang_VERSIONS
	3.6.0
	)

foreach (v ${clang_VERSIONS})
	find_path(clang_BUILD_DIRECTORY lib/clang/${v}/include/stddef.h)
	if (NOT ${clang_BUILD_DIRECTORY} STREQUAL clang_BUILD_DIRECTORY-NOTFOUND)
		set(clang_BUILD_INCLUDE_DIRS "${clang_BUILD_DIRECTORY}/lib/clang/${v}/include")
		break()
	endif ()
endforeach ()

function(add_optimized_program name)
	set(sources ${ARGN})
	
	# unoptimized version
	add_executable(${name} ${sources})
	target_link_libraries(${name} bench_utils ${bobox_LIBRARIES} ${ulibpp_LIBRARIES})
	
	# generate target for optimized build
	add_custom_target(${name}_optimize)
	add_dependencies(${name}_optimize ${name} bobopt)
	
	set(optimized_DIR ${CMAKE_CURRENT_BINARY_DIR}/${name}_optimized_sources/)
	
	# copy files to new directory
	add_custom_command(TARGET ${name}_optimize COMMAND ${CMAKE_COMMAND} -E make_directory ${optimized_DIR})
	
	set(optimized_sources)
	foreach (source_file ${sources})
		get_filename_component(file_name ${source_file} NAME)
		add_custom_command(TARGET ${name}_optimize COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${source_file} ${optimized_DIR})
		list(APPEND optimized_sources ${optimized_DIR}/${file_name})
	endforeach ()
	
	# generate compilation database
	get_directory_property(include_dirs INCLUDE_DIRECTORIES)
	list(APPEND include_dirs ${clang_BUILD_INCLUDE_DIRS})	
	
	exclude_headers(sources_only ${optimized_sources})
	set(compilation_database ${optimized_DIR}/compile_commands.json)
	add_custom_command(TARGET ${name}_optimize
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
		add_custom_command(TARGET ${name}_optimize COMMAND ${bobopt_executable} ${bobopt_ADDITIONAL_ARGUMENTS} -build ${source})
	endforeach ()
	
	# optimized build
	set_source_files_properties(${optimized_sources} PROPERTIES GENERATED true)
	add_executable(${name}_optimized ${optimized_sources})
	target_link_libraries(${name}_optimized bench_utils ${bobox_LIBRARIES} ${ulibpp_LIBRARIES})
	add_dependencies(${name}_optimized ${name}_optimize)
	
	if (BOBOPT_FOLDERS)
		set_property(TARGET ${name} PROPERTY FOLDER ${BENCHMARKS_FOLDER}/${name})
		set_property(TARGET ${name}_optimize PROPERTY FOLDER ${BENCHMARKS_FOLDER}/${name})
		set_property(TARGET ${name}_optimized PROPERTY FOLDER ${BENCHMARKS_FOLDER}/${name})
	endif ()
endfunction()

# prefetch optimization method benchmark.
set(bobopt_benchmarks_prefetch_SOURCES
	prefetch/bench_prefetch.hpp
	prefetch/main.cpp
	)

add_optimized_program(bench_prefetch ${bobopt_benchmarks_prefetch_SOURCES})

# prefetch optimization method with attachment to the end of bodies.
set(bobopt_benchmarks_prefetch_attach_SOURCES
	prefetch/bench_prefetch.hpp
	prefetch/main.cpp
	)

set(bobopt_ADDITIONAL_ARGUMENTS -c ${CMAKE_CURRENT_SOURCE_DIR}/prefetch/attach.cfg)
add_optimized_program(bench_prefetch_attach ${bobopt_benchmarks_prefetch_attach_SOURCES})
set(bobopt_ADDITIONAL_ARGUMENTS)

# yield optimization method benchmark.
set(bobopt_benchmarks_yield_SOURCES
	yield/bench_yield.hpp
	yield/main.cpp
	)

add_optimized_program(bench_yield ${bobopt_benchmarks_yield_SOURCES})
