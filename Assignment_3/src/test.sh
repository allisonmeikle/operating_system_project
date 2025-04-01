#!/bin/bash
MAIN_DIR="$HOME/Desktop/McGill/COMP310/operating_system_project/Assignment_3/src"
TEST_DIR="$HOME/Desktop/McGill/COMP310/operating_system_project/Assignment_3/test_cases"
OUTPUT_DIR="$MAIN_DIR/test-outputs"

# Initialize counters
total_tests=0
passed_tests=0
failed_tests=()

# List of test case names without extension
tests=(tc1 tc2 tc3 tc4 tc5)

# Ensure output dir exists & clear old outputs
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

for test_name in "${tests[@]}"; do
    total_tests=$((total_tests + 1))

    test_input="${TEST_DIR}/${test_name}.txt"
    expected_output="${TEST_DIR}/${test_name}_result.txt"
    my_output="${OUTPUT_DIR}/${test_name}_output.txt"

    # Extract frame and variable sizes from expected output
    FRAME_SIZE=$(grep -m1 "Frame Store Size" "$expected_output" | sed -E 's/.*Frame Store Size = ([0-9]+);.*/\1/')
    VAR_SIZE=$(grep -m1 "Variable Store Size" "$expected_output" | sed -E 's/.*Variable Store Size = ([0-9]+).*/\1/')
    FRAME_SIZE=${FRAME_SIZE:-10}
    VAR_SIZE=${VAR_SIZE:-10}

    echo -e "\n⏳ Recompiling before running test: $test_name (frames=$FRAME_SIZE, vars=$VAR_SIZE)..."
    cd "$MAIN_DIR"
    make clean > /dev/null 2>&1
    make mysh "framesize=$FRAME_SIZE" "varmemsize=$VAR_SIZE" > /dev/null 2>&1
    cd "$TEST_DIR"

    echo -e "\n▶️ Running test: $test_name"
    echo "-------------------------------------------"
    "$MAIN_DIR/mysh" < "$test_input" | tee "$my_output"
    echo "-------------------------------------------"

    if diff -q -b -B -w "$my_output" "$expected_output" > /dev/null; then
        echo "✅ Test $test_name PASSED"
        passed_tests=$((passed_tests + 1))
    else
        echo "❌ Test $test_name FAILED"
        failed_tests+=("$test_name")

        echo -e "\n📊 Side-by-side comparison (Expected | Yours):"
        echo "-------------------------------------------"
        pr -m -t -w 200 "$expected_output" "$my_output"
        echo "-------------------------------------------"

        echo -e "\n🔍 Whitespace-insensitive diff view:"
        echo "-------------------------------------------"
        diff -u -b -B -w "$expected_output" "$my_output" | sed 's/^/    /'
        echo "-------------------------------------------"
    fi
done

# Final summary
echo -e "\n🧾 TEST SUMMARY"
echo "Total tests: $total_tests"
echo "Passed: $passed_tests"
echo "Failed: $((total_tests - passed_tests))"

if [ ${#failed_tests[@]} -ne 0 ]; then
    echo "❌ Failed Tests: ${failed_tests[*]}"
fi
