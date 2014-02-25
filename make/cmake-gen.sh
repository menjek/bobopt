#!/bin/sh

# Initial settings
SCRIPT_DIR=${PWD}
SOURCE_DIR="${SCRIPT_DIR}/.."

# Arguments
GENERATOR=$1
BUILD_TEMPLATE=$2

if [ -z "$GENERATOR" ]; then
    echo "(ERROR) Usage: cmake-gen.sh [generator-name]"
    exit 1
fi

if [ -z "$BUILD_TEMPLATE" ]; then
    echo "(ERROR) Usage: cmake-gen.sh [generator-name]"
    exit 2
fi

if [ -z "$BUILD_TEMPLATE" ]; then
    BUILD_TEMPLATE="default.cmake"
fi

BUILD_DIR="${SOURCE_DIR}/../build/"

mkdir -p ${BUILD_DIR} 2> /dev/null
cd ${BUILD_DIR}

shift
shift
cmake -G "${GENERATOR}" -C "${SCRIPT_DIR}/${BUILD_TEMPLATE}" "${SOURCE_DIR}" $@

cd ${SCRIPT_DIR}

