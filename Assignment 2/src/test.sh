MAIN_DIR="$HOME/operating-systems-w25/project/src"
TEST_DIR="$HOME/operating-systems-w25/A2/test-cases"
OUTPUT_DIR="$MAIN_DIR/test-outputs"

# Initialize counters
total_tests=0
passed_tests=0
failed_tests=()

#tests=("T_background" "T_RR30_2")
tests=("T_RR30_2")

for test_name in "${tests[@]}"; do
    total_tests=$((total_tests + 1))

    test_input="${TEST_DIR}/${test_name}.txt"
    expected_output1="${TEST_DIR}/${test_name}_result.txt"
    my_output="${OUTPUT_DIR}/${test_name}_output.txt"

    # Recompile the program before each test case
    echo "Recompiling before running test: $test_name..."
    cd "$MAIN_DIR"
    make clean && make
    cd "$TEST_DIR"

    # Run the program in batch mode with input redirection and capture the output
    echo "Running test: $test_name"
    "${MAIN_DIR}/mysh" < "$test_input"
done

