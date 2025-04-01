#!/bin/bash

# Define directories
MAIN_DIR="$HOME/operating-systems-w25/project/src"
TEST_DIR="$HOME/operating-systems-w25/A2/test-cases"
OUTPUT_DIR="$MAIN_DIR/test-outputs"

# Initialize counters
total_tests=0
passed_tests=0
failed_tests=()

# Loop through all test cases
for test_input in "$TEST_DIR"/*.txt; do
    # Skip expected result files
    if [[ "$test_input" == *_result.txt || "$test_input" == *_result2.txt ]]; then
    #if [[ "$(basename "$test_input")" != "T_RR4.txt" ]]; then
        continue
    fi

    total_tests=$((total_tests + 1))

    # Extract test name (e.g., "test1" from "test1.txt")
    test_name=$(basename "$test_input" .txt)
    expected_output1="${TEST_DIR}/${test_name}_result.txt"
    expected_output2="${TEST_DIR}/${test_name}_result2.txt"
    my_output="${OUTPUT_DIR}/${test_name}_output.txt"

    # Recompile the program before each test case
    echo "Recompiling before running test: $test_name..."
    cd "$MAIN_DIR"
    make clean && make
    cd "$TEST_DIR"

    # Run the program in batch mode with input redirection
    echo "Running test: $test_name"
    "${MAIN_DIR}/mysh" < "$test_input" > "$my_output"
    if [ $? -ne 0 ]; then
        echo "Segmentation fault occurred while running test $test_name."
        failed_tests+=("$test_name")
        continue
    fi
    
    # Initialize result variables
    result1=1
    result2=1

    # Check if expected result file exists
    if [[ -f "$expected_output1" ]]; then
        diff -q "$my_output" "$expected_output1" > /dev/null
        result1=$?
    fi

    if [[ -f "$expected_output2" ]]; then
        diff -q "$my_output" "$expected_output2" > /dev/null
        result2=$?
    fi

    # A test passes if it matches *either* expected result file
    if [[ $result1 -eq 0 || $result2 -eq 0 ]]; then
        echo "✅ Test $test_name PASSED"
        passed_tests=$((passed_tests + 1))
    else
        echo "❌ Test $test_name FAILED"
        failed_tests+=("$test_name")
        echo "Expected Output:"
        cat "$expected_output1"
        if [[ -f "$expected_output2" ]]; then
            echo "OR"
            cat "$expected_output2"
        fi
        echo "Actual Output:"
        cat "$my_output"
    fi
done

# Print final summary
echo "=================================="
echo "Total Tests: $total_tests"
echo "Passed: $passed_tests"
echo "Failed: $((total_tests - passed_tests))"
if [ ${#failed_tests[@]} -ne 0 ]; then
    echo "Failed Tests:"
    for test in "${failed_tests[@]}"; do
        echo "- $test"
    done
fi
echo "=================================="

# Clear output directory
rm -rf "$OUTPUT_DIR"/*

# Return appropriate exit code
if [[ $passed_tests -eq $total_tests ]]; then
    exit 0  # All tests passed
else
    exit 1  # Some tests failed
fi
done