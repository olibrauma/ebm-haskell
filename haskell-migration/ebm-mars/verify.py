import sys
import glob

def parse_dat_file(path):
    with open(path, 'r') as f:
        lines = f.readlines()
    
    # Header format: loop:0,season:604800,P_air:0.007,P_ice:0,P_rego:0,T_sub:148.973
    header = lines[0].strip().replace('loop:', '').replace('season:', '').replace('P_air:', '').replace('P_ice:', '').replace('P_rego:', '').replace('T_sub:', '')
    header_vals = [float(x) for x in header.split(',')]
    
    # Data format: 0 174.965 2420.27 183.18 5.73347e-12 183.18 5.73347e-12
    data_vals = []
    for line in lines[1:]:
        if not line.strip(): continue
        data_vals.append([float(x) for x in line.split(',') if x.strip() or line.split() if x])
    # The Haskell format might use comma or space. We split by space or comma.
    
    return header_vals, data_vals

def compare_files(golden, test):
    h_gold, d_gold = parse_dat_file(golden)
    h_test, d_test = parse_dat_file(test)
    
    failed = False
    
    for i, (g, t) in enumerate(zip(h_gold, h_test)):
        diff = abs(g - t)
        if diff > 1e-10:
            print(f"Header mismatch at index {i}: gold={g}, test={t}, diff={diff}")
            failed = True
            
    for row_idx, (r_gold, r_test) in enumerate(zip(d_gold, d_test)):
        for col_idx, (g, t) in enumerate(zip(r_gold, r_test)):
            diff = abs(g - t)
            if diff > 1e-10:
                print(f"Data mismatch at row {row_idx}, col {col_idx}: gold={g}, test={t}, diff={diff}")
                failed = True
                if diff > 1e-7: return False # short circuit
    return not failed

golden_files = sorted(glob.glob("golden-master/standard/dump_*.dat"))
test_files = sorted(glob.glob("test_output/dump_*.dat"))

if not test_files:
    print("NO TEST FILES FOUND! Simulation did not run or failed silently.")
    sys.exit(1)

all_passed = True
for g, t in zip(golden_files, test_files):
    print(f"Verifying {t} against {g}...")
    if not compare_files(g, t):
        all_passed = False
        print("FAILED!")
    else:
        print("PASSED.")

if all_passed:
    print("✓ ALL TESTS PASSED.")
    sys.exit(0)
else:
    sys.exit(1)
