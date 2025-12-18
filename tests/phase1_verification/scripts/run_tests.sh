#!/bin/bash

# スクリプトが中断された際に子プロセスを確実に終了させる
trap 'kill 0' EXIT

SCRIPT_DIR="tests/phase1_verification/scripts"
ORIGIN_DIR="tests/phase1_verification/results/origin"
REFACTORED_DIR="tests/phase1_verification/results/refactored"

echo "=== Phase 1 Verification ==="

if [ ! -d "$ORIGIN_DIR" ]; then
    echo "Error: Origin data not found. Run $SCRIPT_DIR/generate_origin.sh first."
    exit 1
fi

echo "Step 1: Generating results from refactored code..."
bash "$SCRIPT_DIR/generate_refactored.sh"

echo "Step 2: Comparing results..."
# 各ディレクトリごとにループ回して比較
ALL_PASS=true
for case_dir in $(ls "$ORIGIN_DIR"); do
    echo "Verifying $case_dir..."
    python3 "$SCRIPT_DIR/verify_results.py" "$ORIGIN_DIR/$case_dir" "$REFACTORED_DIR/$case_dir"
    if [ $? -ne 0 ]; then
        ALL_PASS=false
    fi
done

if [ "$ALL_PASS" = true ]; then
    echo "=== ALL TESTS PASSED ==="
    exit 0
else
    echo "=== SOME TESTS FAILED ==="
    exit 1
fi
