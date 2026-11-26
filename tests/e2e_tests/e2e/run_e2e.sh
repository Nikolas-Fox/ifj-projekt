#!/usr/bin/env bash
# ==============================================================================
# AI Generated (Gemini)
# Prompt: "Vygeneruj mi skript na spúšťanie E2E testov pre celý prekladač, 
#          ktorý skontroluje návratové kódy, výstup z interpretu a voliteľne 
#          aj úniky pamäte cez Valgrind."
# ==============================================================================

COMPILER="./compiler"
INTERPRETER="./ic26int"
TEST_DIR="tests/e2e_tests/e2e"

USE_VALGRIND=0
if [ "$1" == "--valgrind" ]; then
    USE_VALGRIND=1
fi

if [ ! -x "$COMPILER" ]; then
    echo "ERROR: Compiler binary '$COMPILER' does not exist or is not executable."
    echo "Please build the project first (e.g., run 'make')."
    exit 1
fi

PASSED=0
FAILED=0

run_test() {
    local src_file="$1"
    local expected_rc="$2"
    local base="${src_file%.src}"
    local tmp_code
    local tmp_out
    local tmp_vg
    tmp_code=$(mktemp)
    tmp_out=$(mktemp)
    tmp_vg=$(mktemp)
    trap 'rm -f "$tmp_code" "$tmp_out" "$tmp_vg"' RETURN

    if [ "$USE_VALGRIND" -eq 1 ]; then
        valgrind --leak-check=full --error-exitcode=199 --log-file="$tmp_vg" \
            "$COMPILER" < "$src_file" > "$tmp_code" 2>/dev/null
    else
        "$COMPILER" < "$src_file" > "$tmp_code" 2>/dev/null
    fi
    local actual_rc=$?

    if [ "$USE_VALGRIND" -eq 1 ] && [ "$actual_rc" -eq 199 ]; then
        echo "[FAIL - MEM] $src_file: Valgrind detected a memory leak or error!"
        cat "$tmp_vg"
        FAILED=$((FAILED + 1))
        return
    fi

    if [ "$actual_rc" -ne "$expected_rc" ]; then
        echo "[FAIL - RC]  $src_file: expected RC=$expected_rc, actual RC=$actual_rc"
        FAILED=$((FAILED + 1))
        return
    fi

    if [ "$expected_rc" -eq 0 ] && [ -x "$INTERPRETER" ]; then
        if [ -f "${base}.in" ]; then
            "$INTERPRETER" "$tmp_code" < "${base}.in" > "$tmp_out" 2>/dev/null
        else
            "$INTERPRETER" "$tmp_code" > "$tmp_out" 2>/dev/null
        fi
        local int_rc=$?

        if [ "$int_rc" -ne 0 ]; then
            echo "[FAIL - INT] $src_file: interpreter failed with RC=$int_rc"
            FAILED=$((FAILED + 1))
            return
        fi

        if [ -f "${base}.out" ] && ! diff -u "${base}.out" "$tmp_out" > /dev/null; then
            echo "[FAIL - OUT] $src_file: interpreter output does not match ${base}.out"
            diff -u "${base}.out" "$tmp_out"
            FAILED=$((FAILED + 1))
            return
        fi
    fi

    echo "[PASS]       $src_file"
    PASSED=$((PASSED + 1))
}

echo "=== Running E2E tests (Valgrind: $USE_VALGRIND) ==="
if [ ! -x "$INTERPRETER" ]; then
    echo "(Note: Interpreter '$INTERPRETER' not found, checking compiler return codes only)"
fi

for f in "$TEST_DIR"/valid/*.src;        do [ -e "$f" ] && run_test "$f" 0; done
for f in "$TEST_DIR"/err1_lex/*.src; do
    [ -e "$f" ] || continue
    case "$(basename "$f")" in
        01_tab_indent.src|02_unterminated_str.src)
            run_test "$f" 2
            ;;
        *)
            run_test "$f" 1
            ;;
    esac
done
for f in "$TEST_DIR"/err2_syn/*.src;     do [ -e "$f" ] && run_test "$f" 2; done
for f in "$TEST_DIR"/err3_undef/*.src;   do [ -e "$f" ] && run_test "$f" 3; done
for f in "$TEST_DIR"/err4_redef/*.src;   do [ -e "$f" ] && run_test "$f" 4; done
for f in "$TEST_DIR"/err5_params/*.src;  do [ -e "$f" ] && run_test "$f" 5; done
for f in "$TEST_DIR"/err6_ret/*.src;     do [ -e "$f" ] && run_test "$f" 6; done
for f in "$TEST_DIR"/err7_types/*.src;   do [ -e "$f" ] && run_test "$f" 7; done
for f in "$TEST_DIR"/err10_other/*.src;  do [ -e "$f" ] && run_test "$f" 10; done

echo "----------------------------------------"
echo "Result: $PASSED passed, $FAILED failed"
[ "$FAILED" -eq 0 ]
