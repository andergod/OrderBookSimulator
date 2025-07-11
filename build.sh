#!/bin/bash

readonly DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
readonly BUILD_DIR="$DIR/build"
readonly OUTPUT_FILE="$DIR/build_output.txt"

print_help() {
  cat <<EOF
Usage: $(basename "$0") [BUILD_TYPE] [NUM_THREADS]

Options:
  BUILD_TYPE     Optional. Specify the build type: Debug (default) or Release.
  NUM_THREADS    Optional. Specify the number of parallel build threads.
                 If not provided, the script will auto-detect and use (nproc - 1).

Flags:
  -h, --help      Show this help message and exit.

Examples:
  $(basename "$0")                 # Debug build with auto thread detection
  $(basename "$0") Release         # Release build with auto thread detection
  $(basename "$0") Debug 8         # Debug build with 8 threads
EOF
  exit 0
}

error_exit() {
  echo "Error: $1"
  popd >/dev/null 2>&1
  exit 1
}

# Handle --help or -h
if [[ "$1" == "--help" || "$1" == "-h" ]]; then
  print_help
fi

# Default to "Debug" if no arguments are provided
BUILD_TYPE="${1:-Debug}"

if [[ "$BUILD_TYPE" != "Debug" && "$BUILD_TYPE" != "Release" ]]; then
  error_exit "Invalid argument: $1. Expected 'Debug' or 'Release'. Use --help for usage."
fi

# Optional second argument: number of threads
if [[ -n "$2" ]]; then
  if [[ "$2" =~ ^[0-9]+$ && "$2" -gt 0 ]]; then
    NUM_PROC="$2"
  else
    error_exit "Invalid number of threads: $2"
  fi
else
  # Auto-detect threads if not specified
  NUM_PROC=$(nproc)
  if [[ -z "$NUM_PROC" || "$NUM_PROC" -le 0 ]]; then
    NUM_PROC=4
  fi
  if [[ "$NUM_PROC" -gt 1 ]]; then
    NUM_PROC=$((NUM_PROC - 1))
  fi
fi

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
echo ""

# Redirect both stdout and stderr to the output file
{
  cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" .. || error_exit "CMake configuration failed"
  cmake --build . --parallel "$NUM_PROC" || error_exit "CMake build failed"
} 2>&1 | tee "$OUTPUT_FILE"

popd || error_exit "Failed to return to the original directory"

exit 0
