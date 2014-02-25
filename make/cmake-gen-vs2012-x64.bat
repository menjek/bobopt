@echo off

@set BUILD_TEMPLATE=%1

if [%1] == [] (
	set BUILD_TEMPLATE=default.cmake
)

@set BUILD_TEMPLATE_DIR=x86_64-windows-vs2012

call cmake-gen.bat "Visual Studio 11 Win64" "%BUILD_TEMPLATE_DIR%" "%BUILD_TEMPLATE%"