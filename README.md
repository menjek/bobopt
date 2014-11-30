Basic information
================================================================================
The front-end optimizer for the Bobox framework:
http://www.ksi.mff.cuni.cz/bobox/

This package contains source files for the optimizer and three benchmarks for
two different optimization methods.

Projects
================================================================================
CMake version at least 2.6 should be used to generate projects.
The "make" directory contains batches to run CMake and generate projects for
MSVC, Clang or GCC. Projects are generated to the ${root}/../build directory.

Usage of MSVC batches:
cmake-gen-vs2012-win32.bat (Init cache)
cmake-gen-vs2012-x64.bat (Init cache)
cmake-gen-vs2013-win32.bat (Init cache)
cmake-gen-vs2013-x64.bat (Init cache)

Batches take one optional argument, CMake init cache file.

Usage of GCC, Clang batches:
cmake-gen-make-clang.sh (Debug|Release) (Init cache)
cmake-gen-make-gcc.sh (Debug|Release) (Init cache)

Batches take two optional arguments. The first parameter is configuration
(Debug|Release). The second parameter is CMake init cache file.

CMake init cache files set some CMake variables that configure what parts
of source code get into project.
default.cmake - Whole project in folders.
no-benchmarks.cmake - Only the optimizer code with folders.
no-folders.cmake - Whole project without folders.

Note: Folders are MSVC only feature.

CMake files use CMAKE_PREFIX_PATH to find libraries, i.e., Clang/LLVM and
Bobox.

For Clang, path expects following directory structure:
(path)/llvm - LLVM and Clang source code.
(path)/build - Build directory of Clang/LLVM.

For Bobox, path expects following directory structure:
(path)/bobox - Bobox source code
(path)/extlibs
(path)/ulibpp