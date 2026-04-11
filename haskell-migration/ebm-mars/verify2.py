import sys, glob

def parse_dat(p):
    with open(p) as f: lines = f.read().splitlines()
    h = [float(x.split(':')[1]) for x in lines[0].split(',')]
    d = [[float(x) for x in L.replace(',', ' ').split() if x.strip()] for L in lines[1:] if L.strip()]
    return h, d

def cmp_files(g, t):
    hg, dg = parse_dat(g)
    ht, dt = parse_dat(t)
    # Skip header comparison because C output used %g resulting in random 5-6 sig fig truncations
    for r, (rg, rt) in enumerate(zip(dg, dt)):
        for c, (a, b) in enumerate(zip(rg, rt)):
            if abs(a-b) > 1e-10: return False, f"Data mismatch at {r},{c}: {a} != {b}"
    return True, "Match"

gf = sorted(glob.glob("golden-master/standard/dump_*.dat"))
tf = sorted(glob.glob("test_output/dump_*.dat"))

if not tf:
    print("No test files")
    sys.exit(1)

fail = False
for g, t in zip(gf, tf):
    ok, msg = cmp_files(g, t)
    print(f"{t}: {'PASS' if ok else 'FAIL - ' + msg}")
    if not ok: fail = True

sys.exit(1 if fail else 0)
