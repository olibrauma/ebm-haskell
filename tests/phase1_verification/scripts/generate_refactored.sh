#!/bin/bash

# スクリプトが中断された際に子プロセスを確実に終了させる
trap 'kill 0' EXIT

# パラメータグリッドの定義
P_TOTALS=("0.007" "0.1" "1.0")
OBLS=("0.0" "25.19" "45.0" "90.0")
ALPHAS=("0.0" "30.0" "45.0")

BASE_DIR="tests/phase1_verification/results/refactored"
mkdir -p "$BASE_DIR"

# バイナリの存在確認
if [ ! -f "./test" ]; then
    echo "Error: ./test binary not found. Compile test.c first."
    exit 1
fi

for p in "${P_TOTALS[@]}"; do
    for o in "${OBLS[@]}"; do
        for a in "${ALPHAS[@]}"; do
            CASE_DIR="${BASE_DIR}/p${p}_o${o}_a${a}"
            mkdir -p "$CASE_DIR"
            echo "Running: P=$p, Obl=$o, Alpha=$a -> $CASE_DIR"
            # タイムアウトを設定し、万が一のハングを防止
            timeout 30s ./test "$p" "$o" "$a" "$CASE_DIR" > /dev/null 2>&1
            if [ $? -eq 124 ]; then
                echo "Warning: Case P=$p, O=$o, A=$a timed out."
            fi
        done
    done
done

echo "All refactored cases generated."
