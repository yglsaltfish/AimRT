#!/bin/bash

clang_format_version=$(clang-format --version 2>&1)

if [[ $clang_format_version == *"version 15"* ]]; then
    echo "use clang-format-15"
else
    echo "error, can not find clang-format-15"
    exit 1
fi

# clang-format, version v15 is required
find ./src -regex '.*\.cc\|.*\.h\|.*\.proto' -and -not -regex '.*\.pb\.cc\|.*\.pb\.h' | xargs clang-format -i --style=file
echo "clang-format done"

# cmake-format, apt install cmake-format
find ./ -regex '.*\.cmake\|.*CMakeLists\.txt$' -and -not -regex '\./build/.*\|\./document/.*' | xargs cmake-format -c ./.cmake-format.py -i
echo "cmake-format done"

# autopep8, apt install python3-autopep8
find ./ -regex '.*\.py' -and -not -regex '\./build/.*\|\./document/.*' | xargs autopep8 -i --global-config ./.pycodestyle
echo "python format done"
