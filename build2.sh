#!/bin/bash

readonly DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
readonly BUILD_DIR="$DIR/build"
readonly OUTPUT_FILE="$DIR/build_output.txt"

print_help() {
  cat <<EOF
Usage: $(basename "$0") [BUILD_TYPE] [NUM_THREADS] [-G GENERATOR] [-A ARCHITECTURE]

Options:
  BUILD_TYPE     Optional. Specify the build type: Debug (default) or Release.
  NUM_THREADS    Optional. Number of parallel build threads. Defaults to (nproc - 1).
  -G GENERATOR   Optional. CMake generator (e.g., Ninja, Unix Makefiles, Visual Studio 17 2022).
  -A ARCHITECTURE Optional. Architecture for Visual Studio generators (e.g., Win32, x64, ARM64).

Flags:
  -h, --help     Show this help message and exit.

Examples:
  $(basename "$0")                            # Debug build, auto threads
  $(basename "$0") Release                    # Release build, auto threads
  $(basename "$0") Debug 8                    # Debug build, 8 threads
  $(basename "$0") Release 4 -G Ninja         # Release build, 4 threads, Ninja generator
  $(basename "$0") Debug -G "Visual Studio 17 2022" -A x64  # Debug build, Visual Studio, x64
EOF
  exit 0
}

error_exit() {
  echo "Error: $1"
  popd >/dev/null 2>&1
  exit 1
}

# Parse arguments
BUILD_TYPE="Debug"
NUM_PROC=""
GENERATOR=""
ARCHITECTURE=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --help|-h) print_help ;;
    -G) shift; GENERATOR="$1" ;;
    -A) shift; ARCHITECTURE="$1" ;;
    Debug|Release) BUILD_TYPE="$1" ;;
    [0-9]*) NUM_PROC="$1" ;;
    *) error_exit "Invalid argument: $1. Use --help for usage." ;;
  esac
  shift
done

# Validate build type
if [[ "$BUILD_TYPE" != "Debug" && "$BUILD_TYPE" != "Release" ]]; then
  error_exit "Invalid build type: $BUILD_TYPE. Expected 'Debug' or 'Release'."
fi

# Validate number of threads
if [[ -n "$NUM_PROC" ]]; then
  if [[ ! "$NUM_PROC" =~ ^[0-9]+$ || "$NUM_PROC" -le 0 ]]; then
    error_exit "Invalid number of threads: $NUM_PROC"
  fi
else
  NUM_PROC=$(nproc)
  if [[ -z "$NUM_PROC" || "$NUM_PROC" -le 0 ]]; then
    NUM_PROC=4
  fi
  if [[ "$NUM_PROC" -gt 1 ]]; then
    NUM_PROC=$((NUM_PROC - 1))
  fi
fi

# Clean or create build directory
if [ -f "$OUTPUT_FILE" ]; then
  rm -f "$OUTPUT_FILE" || error_exit "Failed to delete existing output file"
fi

if [ ! -d "$BUILD_DIR" ]; then
  mkdir "$BUILD_DIR" || error_exit "Failed to create build directory"
fi

pushd "$BUILD_DIR" || error_exit "Failed to change to build directory"

echo ""
echo "This is a $BUILD_TYPE build."

echo "Building with $NUM_PROC threads."
[[ -n "$GENERATOR" ]] && echo "Using generator: $GENERATOR"
[[ -n "$ARCHITECTURE" ]] && echo "Architecture: $ARCHITECTURE"
echo ""

# Build CMake command
CMAKE_CMD="cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE"
[[ -n "$GENERATOR" ]] && CMAKE_CMD="$CMAKE_CMD -G \"$GENERATOR\""
[[ -n "$ARCHITECTURE" ]] && CMAKE_CMD="$CMAKE_CMD -A $ARCHITECTURE"
CMAKE_CMD="$CMAKE_CMD .."

# Execute CMake and build
{
  eval "$CMAKE_CMD" || error_exit "CMake configuration failed"
  cmake --build . --config "$BUILD_TYPE" --parallel "$NUM_PROC" || error_exit "CMake build failed"
} 2>&1 | tee "$OUTPUT_FILE"

popd || error_exit "Failed to return to the original directory"

exit 0
