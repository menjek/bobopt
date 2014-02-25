#!/bin/sh

which gcc &> /dev/null
if [ $? -ne 0 ]; then
   echo "Couldn't detect C compiler \"gcc\" on current system."
   exit 1
fi

which g++ &> /dev/null
if [ $? -ne 0 ]; then
   echo "Couldn't detect CXX compiler \"g++\" on current system."
   exit 1
fi

BUILD_TYPE=$1
BUILD_TEMPLATE=$2

if [ -z "$BUILD_TYPE" ]; then
    BUILD_TYPE="Debug"
fi

if [ -z "$BUILD_TEMPLATE" ]; then
    BUILD_TEMPLATE="default.cmake"
fi

echo "Generating \"${BUILD_TYPE}\" configuration with init cache \"${BUILD_TEMPLATE}\""

./cmake-gen.sh "Unix Makefiles" ${BUILD_TEMPLATE} -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
