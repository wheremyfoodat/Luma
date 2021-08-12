#!/usr/bin/env bash

echo "Trying to build..."
g++ test.cc -std=c++17 -o test.out && ./test.out

if [ $? -ne 0 ]; then
    echo "Test 1 failed"
    exit -1
fi

echo "Done testing!"