import os
import subprocess
import math

# Full 80-case Sweep Parameters
pressures = [0.001, 0.007, 0.1, 1.0, 5.0]
obliquities = [0.0, 25.0, 45.0, 90.0]
alphas = [0.0, 10.0, 30.0, 45.0]

c_binary = "/home/takeru/git/EBM-on-Mars/ebm-c-refactored/test_modular"
output_root = "/home/takeru/git/EBM-on-Mars/data/golden-master"

def main():
    os.makedirs(output_root, exist_ok=True)
    total = len(pressures) * len(obliquities) * len(alphas)
    count = 0

    print(f"Starting High-Precision Golden Master Generation ({total} cases)...")

    for p in pressures:
        for o in obliquities:
            for a in alphas:
                count += 1
                case_dir = os.path.join(output_root, f"p{p}_o{o}_a{a}")
                os.makedirs(case_dir, exist_ok=True)
                
                print(f"[{count:02d}/{total}] Generating {case_dir}...", end="", flush=True)
                
                # Command: binary P obl alpha out_dir
                # Note: test_modular expected P obl alpha out_dir
                # argv[1]=P, argv[2]=obl, argv[3]=alpha, argv[4]=out_dir
                cmd = [c_binary, str(p), str(o), str(a), case_dir]
                
                try:
                    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                    print("DONE")
                except subprocess.CalledProcessError as e:
                    print(f"FAILED (exit code {e.returncode})")

    print(f"Generation complete. Results at {output_root}")

if __name__ == "__main__":
    main()
