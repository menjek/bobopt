@echo off

@rem INITIALIZATION
@set CURRENT_DIR=%CD%
@set BATCH_DIR=%~dp0
@set ROOT_DIR=%~dp0\..
@set MAKE_PROGRAM="MSBuild.exe"

@rem PARAMETERS HANDLING
@set GENERATOR=%1
@set BUILD_TRIPLET_TEMPLATE=%2
@set BUILD_TEMPLATE=%3

if [%GENERATOR%] == [] (
	goto error_parameters
)

if [%BUILD_TRIPLET_TEMPLATE%] == [] (
	goto error_parameters
)

if [%BUILD_TEMPLATE%] == [] (
	set BUILD_TEMPLATE="default.cmake"
)

@rem ADDITIONAL SETTINGS
@set GENERATOR_TOOLCHAIN=""
if %GENERATOR% == "Visual Studio 11" (
	set GENERATOR_TOOLCHAIN=-T "v120_CTP_Nov2012"
)
if %GENERATOR% == "Visual Studio 11 Win64" (
	set GENERATOR_TOOLCHAIN=-T "v120_CTP_Nov2012"
)

@rem DIRECTORIES
set SOURCES_DIR=%ROOT_DIR%
set BUILD_DIR=%ROOT_DIR%\..\build\%BUILD_TRIPLET_TEMPLATE%

@rem EXECUTION
mkdir %BUILD_DIR% 2> nul
chdir %BUILD_DIR%

cmake.exe -G %GENERATOR% %GENERATOR_TOOLCHAIN% -C "%BATCH_DIR%\%BUILD_TEMPLATE%" -DCMAKE_MAKE_PROGRAM=%MAKE_PROGRAM% %SOURCES_DIR%

chdir %CURRENT_DIR%

goto exit

@rem FAILURES
:error_parameters
echo (ERROR) Usage: cmake-gen.bat [generator-name] [build-dir-suffix]

:error
pause

:exit
