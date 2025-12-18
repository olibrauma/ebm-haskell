import sys
import os
import math

def load_dump(filepath):
    """ダンプファイルを読み込み、(header, data_rows) の形式で返す"""
    with open(filepath, 'r') as f:
        header = f.readline().strip()
        data = [line.strip() for line in f.readlines()]
    return header, data

def compare_files(file_orig, file_ref):
    """2つのダンプファイルを比較し、誤差があれば報告する"""
    h1, d1 = load_dump(file_orig)
    h2, d2 = load_dump(file_ref)
    
    errors = []
    
    # ヘッダー（スカラ値）の比較
    if h1 != h2:
        # 文字列として不一致な場合、浮動小数点としてパースして誤差をチェック
        kv1 = dict(item.split(':') for item in h1.split(','))
        kv2 = dict(item.split(':') for item in h2.split(','))
        for k in kv1:
            v1, v2 = float(kv1[k]), float(kv2[k])
            if not math.isclose(v1, v2, rel_tol=1e-13, abs_tol=1e-15):
                errors.append(f"Header mismatch in {k}: {v1} != {v2}")

    # データ行（配列値）の比較
    if len(d1) != len(d2):
        errors.append(f"Data length mismatch: {len(d1)} vs {len(d2)}")
    else:
        for i, (l1, l2) in enumerate(zip(d1, d2)):
            v1s = [float(x) for x in l1.split(',')]
            v2s = [float(x) for x in l2.split(',')]
            for j, (v1, v2) in enumerate(zip(v1s, v2s)):
                if not math.isclose(v1, v2, rel_tol=1e-13, abs_tol=1e-15):
                    errors.append(f"Data mismatch at row {i}, col {j}: {v1} != {v2}")
                    if len(errors) > 10: break # 最初の方のミスだけ報告
            if len(errors) > 10: break

    return errors

def verify_all(origin_dir, refactored_dir):
    """ディレクトリ内の全ファイルを検証する"""
    all_ok = True
    for filename in sorted(os.listdir(origin_dir)):
        if not filename.endswith(".dat"): continue
        
        orig_path = os.path.join(origin_dir, filename)
        ref_path = os.path.join(refactored_dir, filename)
        
        if not os.path.exists(ref_path):
            print(f"[FAIL] Missing refactored file: {filename}")
            all_ok = False
            continue
            
        errs = compare_files(orig_path, ref_path)
        if errs:
            print(f"[FAIL] {filename}: {len(errs)} errors found!")
            for e in errs[:3]: print(f"  - {e}")
            all_ok = False
        else:
            print(f"[PASS] {filename}")
            
    return all_ok

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python verify_results.py <origin_dir> <refactored_dir>")
        sys.exit(1)
        
    success = verify_all(sys.argv[1], sys.argv[2])
    sys.exit(0 if success else 1)
