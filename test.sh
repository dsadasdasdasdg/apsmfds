#!/bin/bash

# Check if gcc is installed
if ! command -v gcc &> /dev/null
then
    echo "gcc could not be found, attempting to install it..."
    sudo apt-get update && sudo apt-get install -y gcc
    if ! command -v gcc &> /dev/null
    then
        echo "Failed to install gcc. Please install it manually and try again."
        exit 1
    fi
fi

# Compile the C program
gcc -o hello_test_executable hello.c
if [ $? -ne 0 ]; then
    echo "Compilation failed."
    exit 1
fi

# Run the executable and capture the output
output=$(./hello_test_executable)

# Check the output
if [ "$output" == "hello" ]; then
    echo "Test passed"
else
    echo "Test failed"
    echo "Expected: hello"
    echo "Got: $output"
fi

# Clean up the executable
rm hello_test_executable
