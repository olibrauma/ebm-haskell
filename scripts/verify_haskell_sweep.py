import os
import subprocess
import math

# Grid configuration
pressures = [0.001, 0.007, 0.1, 1.0, 5.0]
obliquities = [0.0, 25.0, 45.0, 90.0]
alphas = [0.0, 10.0, 30.0, 45.0]

haskell_binary = "/home/takeru/git/EBM-on-Mars/ebm-haskell/.stack-work/install/x86_64-linux/adbb68db0091794eff7db37725d73be34e1dd483fcbaa424fe33903096ef0623/9.4.8/bin/ebm-mars-exe"
golden_root = "/home/takeru/git/EBM-on-Mars/data/golden-master"
test_root = "/home/takeru/.gemini/antigravity/scratch/haskell_sweep_final"

def parse_dat(path):
    with open(path, 'r') as f:
        lines = f.readlines()
    header_line = lines[0].strip().replace('loop:', '').replace('season:', '').replace('P_air:', '').replace('P_ice:', '').replace('P_rego:', '').replace('T_sub:', '')
    header = [float(x) for x in header_line.split(',')]
    data = []
    for line in lines[1:]:
        if line.strip():
            # Support both space and comma just in case, though we expect comma
            parts = line.replace(',', ' ').split()
            data.append([float(x) for x in parts])
    return header, data

def compare(h1, d1, h2, d2, tol=1e-10):
    # Header
    for i, (v1, v2) in enumerate(zip(h1, h2)):
        if abs(v1-v2) > tol and abs((v1-v2)/max(abs(v1),abs(v2),1e-10)) > tol:
            return f"Header mismatch at idx {i}: {v1} vs {v2}"
    
    # Data
    if len(d1) != len(d2):
        return f"Row count mismatch: {len(d1)} vs {len(d2)}"
    
    for r, (row1, row2) in enumerate(zip(d1, d2)):
        if len(row1) != len(row2):
            return f"Col count mismatch at row {r}"
        for c, (v1, v2) in enumerate(zip(row1, row2)):
            # Skip index column (0)
            if c == 0: continue
            if abs(v1-v2) > tol and abs((v1-v2)/max(abs(v1),abs(v2),1e-10)) > tol:
                return f"Data mismatch at row {r}, col {c}: {v1} vs {v2}"
    return None

def main():
    os.makedirs(test_root, exist_ok=True)
    total = len(pressures) * len(obliquities) * len(alphas)
    passed = 0
    failed = 0
    
    print(f"Starting Haskell Verification Sweep ({total} cases)...")
    
    for p in pressures:
        for o in obliquities:
            for a in alphas:
                case_id = f"p{p}_o{o}_a{a}"
                h_dir = os.path.join(test_root, case_id)
                g_dir = os.path.join(golden_root, case_id)
                os.makedirs(h_dir, exist_ok=True)
                
                print(f"Testing {case_id}...", end="", flush=True)
                
                # Run Haskell
                cmd = [haskell_binary, str(p), str(o), str(a), h_dir]
                try:
                    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                    
                    # Compare dump_010.dat
                    f_gold = os.path.join(g_dir, "dump_010.dat")
                    f_test = os.path.join(h_dir, "dump_010.dat")
                    
                    if not os.path.exists(f_test):
                        print("CRASH (No output)")
                        failed += 1
                        continue
                        
                    h1, d1 = parse_dat(f_gold)
                    h2, d2 = parse_dat(f_test)
                    
                    err = compare(h1, d1, h2, d2)
                    if err:
                        print(f"FAIL: {err}")
                        failed += 1
                    else:
                        print("PASS")
                        passed += 1
                        
                except Exception as e:
                    print(f"ERROR: {e}")
                    failed += 1
                    
    print("-" * 30)
    print(f"PASSED: {passed}/{total}")
    print(f"FAILED: {failed}/{total}")
    
    if failed == 0:
        print("VERIFICATION SUCCESSFUL: Haskell port matches C modular with high precision.")
    else:
        print("VERIFICATION FAILED: Discrepancies found.")

if __name__ == "__main__":
    main()
