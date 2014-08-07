@echo off

@set BUILD_TEMPLATE=%1

if [%1] == [] (
	set BUILD_TEMPLATE=default.cmake
)

@set BUILD_TEMPLATE_DIR=win32-windows-vs2013

call cmake-gen.bat "Visual Studio 12" "%BUILD_TEMPLATE_DIR%" "%BUILD_TEMPLATE%"