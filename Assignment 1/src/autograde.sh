#!/bin/bash

TEST_DIR="$HOME/operating-systems-w25/A1/test-cases"
OUTPUT_DIR="$HOME/operating-systems-w25/A1/test-outputs"
RUN_DIR="$HOME/operating-systems-w25/A1/test-environment"
CUR_DIR=$(pwd)

echo "Running tests for mysh..."
cd "$RUN_DIR"

# Iterate over all test input files (excluding *_result.txt)
for test_file in "$TEST_DIR"/*.txt; do
    # Skip expected output files (_result.txt)
    if [[ "$test_file" == *_result.txt ]]; then
        continue
    fi

    # Extract test name (without .txt extension)
    test_name=$(basename "$test_file" .txt)

    # Define paths
    actual_output="$OUTPUT_DIR/${test_name}.txt"
    expected_output="$TEST_DIR/${test_name}_result.txt"

    echo "Running test: $test_name"

    # Run your shell and capture output
    "$HOME/operating-systems-w25/project/src/mysh" < "$test_file" > "$actual_output"

    # Compare actual output with expected output
    if diff -q "$actual_output" "$expected_output" > /dev/null; then
        echo "✅ Test $test_name PASSED"
    else
        echo "❌ Test $test_name FAILED"
        echo "Differences:"
        diff "$actual_output" "$expected_output"
    fi
    rm -rf "$RUN_DIR"/*
done

echo "Testing complete!"
cd "$CUR_DIR"
