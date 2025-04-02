#!/bin/bash

# Dynamically resolve project directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/src" && pwd)"
MAIN_DIR="$SCRIPT_DIR"
TEST_DIR="$MAIN_DIR/../test_cases"
OUTPUT_DIR="$MAIN_DIR/../test_outputs"
CUR_DIR=$(pwd)

mkdir -p "$OUTPUT_DIR"
rm -rf "$OUTPUT_DIR"/*
cd "$TEST_DIR" || exit 1

# Initialize counters
total_tests=0
passed_tests=0
failed_tests=()

# Automatically find test input files (excluding _result.txt files)
test_inputs=($(ls "$TEST_DIR"/*.txt | grep -v "_result"))

for test_input_path in "${test_inputs[@]}"; do
    test_name=$(basename "$test_input_path" .txt)
    test_input="$TEST_DIR/${test_name}.txt"
    result1="$TEST_DIR/${test_name}_result.txt"
    my_output="$OUTPUT_DIR/${test_name}_output.txt"

    total_tests=$((total_tests + 1))

    echo -e "\n🧪 Running test: $test_name"

    # Compile or skip if already compiled
    cd "$TEST_DIR" || exit 1

    # Run test
    "$MAIN_DIR/mysh" < "$test_input" > "$my_output" 2>&1

    # Compare output to expected results
    if diff -q "$result1" "$my_output" >/dev/null; then
        echo "✅ Test $test_name PASSED"
        passed_tests=$((passed_tests + 1))
    else
        echo "❌ Test $test_name FAILED"
        failed_tests+=("$test_name")
        echo "🔍 Differences:"
        diff "$my_output" "$result1"
    fi

    # Optional cleanup (non-txt files and empty dirs in test dir)
    find "$TEST_DIR" -type f ! -name "*.txt" -delete
    find "$TEST_DIR" -type d -empty -delete
done

cd "$CUR_DIR"

# 📊 Summary
echo -e "\n===================="
echo "🔍 Test Summary:"
echo "Total: $total_tests"
echo "✅ Passed: $passed_tests"
echo "❌ Failed: $((total_tests - passed_tests))"
echo "===================="

if [ ${#failed_tests[@]} -gt 0 ]; then
    echo -e "\n❌ Failed Tests:"
    for fail in "${failed_tests[@]}"; do
        echo "- $fail"
    done
fi
