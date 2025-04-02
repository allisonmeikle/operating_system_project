#!/bin/bash
# Dynamically resolve project directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/src" && pwd)"
MAIN_DIR="$SCRIPT_DIR"
TEST_DIR="$MAIN_DIR/../test_cases"
OUTPUT_DIR="$MAIN_DIR/../test_outputs"
CUR_DIR=$(pwd)

mkdir -p "$OUTPUT_DIR"

# Initialize counters
total_tests=0
passed_tests=0
failed_tests=()

# Automatically find test input files (excluding _result.txt files)
test_inputs=($(ls "$TEST_DIR"/T_*.txt | grep -v "_result"))

for test_input_path in "${test_inputs[@]}"; do
    test_name=$(basename "$test_input_path" .txt)
    test_input="$TEST_DIR/${test_name}.txt"
    result1="$TEST_DIR/${test_name}_result.txt"
    result2="$TEST_DIR/${test_name}_result2.txt"
    my_output="$OUTPUT_DIR/${test_name}_output.txt"

    total_tests=$((total_tests + 1))

    echo -e "\n🧪 Recompiling and running test: $test_name"
    cd "$MAIN_DIR" || exit 1
    make clean >/dev/null && make >/dev/null

    if [[ ! -f mysh ]]; then
        echo "❌ Compilation failed!"
        failed_tests+=("$test_name (compilation)")
        continue
    fi

    cd "$TEST_DIR" || exit 1

    # Run the test and save output
    "$MAIN_DIR/mysh" < "$test_input" > "$my_output" 2>&1

    # Compare against one or both expected outputs
    if diff -q "$result1" "$my_output" >/dev/null; then
        echo "✅ Test $test_name PASSED (matched result 1)"
        passed_tests=$((passed_tests + 1))
    elif [[ -f "$result2" ]] && diff -q "$result2" "$my_output" >/dev/null; then
        echo "✅ Test $test_name PASSED (matched result 2)"
        passed_tests=$((passed_tests + 1))
    else
        echo "❌ Test $test_name FAILED"
        failed_tests+=("$test_name")
    fi
done
rm -rf "$OUTPUT_DIR"

# Summary
echo -e "\n🔍 Test Summary:"
echo "Total: $total_tests | Passed: $passed_tests | Failed: $((total_tests - passed_tests))"
if [ ${#failed_tests[@]} -gt 0 ]; then
    echo -e "\n❌ Failed Tests:"
    for fail in "${failed_tests[@]}"; do
        echo "- $fail"
    done
fi
