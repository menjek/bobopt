separate_arguments(OPTIMIZED_SOURCES)
separate_arguments(INCLUDE_DIRS)

foreach (dir ${INCLUDE_DIRS})
	set(cmd_include_dirs "${cmd_include_dirs} -I'${dir}'")
endforeach ()

set(platform_macros "-DULIBPP_OS_PLATFORM=3 -DULIBPP_CC_PLATFORM=1 -DULIBPP_CC_VERSION=400")

file(WRITE ${COMPILATION_DATABASE} "[")
foreach (source ${OPTIMIZED_SOURCES})
	get_filename_component(file_name ${source} NAME)

	file(APPEND ${COMPILATION_DATABASE}
"
{
  \"directory\": \"${OPTIMIZED_DIR}\",
  \"command\": \"clang++ -std=c++11 ${cmd_include_dirs} ${platform_macros} -DULIBPP_NEED_TO_USE_UPP_SHARED_PTR -ULIBPP_CC_STDCPP11 -DNDEBUG '${file_name}'\",
  \"file\": \"${file_name}\"
},
")
endforeach ()
file(APPEND ${COMPILATION_DATABASE} "]")