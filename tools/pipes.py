#!/usr/bin/env python3
# Show how busy each CPU pipe (execution port) is in a version's hot inner loop.
#
#   python3 tools/pipes.py versions/v5.cpp            # both models
#   python3 tools/pipes.py versions/v5.cpp --raw      # also print full llvm-mca output
#
# Steps: compile to assembly -> pick the innermost loop with the most float math
# -> run llvm-mca -> print utilization per pipe group.
#
# Models: llvm-mca has no real M3 model. "apple-m3" falls back to Apple's 2013
# Cyclone (3 vector pipes). "neoverse-v2" has 4 x 128-bit vector pipes, closer to M3.
# Treat both as estimates; the measured time is the ground truth.
import re, subprocess, sys, tempfile, os
from collections import defaultdict

MCA = "/opt/homebrew/opt/llvm/bin/llvm-mca"
CXX = ["clang++", "-std=c++17", "-O3", "-Xpreprocessor", "-fopenmp",
       "-I/opt/homebrew/opt/libomp/include", "-S", "-o", "-"]
MODELS = ["apple-m3", "neoverse-v2"]
ITER = 300

# What each pipe group does, by llvm-mca resource name.
DESCRIBE = {
    "CyUnitV": "vector / float (add, min, shuffle)", "CyUnitLS": "load / store",
    "CyUnitI": "integer ALU", "CyUnitB": "branch", "CyUnitVM": "vector multiply",
    "CyUnitVC": "vector crypto", "CyUnitVD": "vector divide", "CyUnitIS": "integer shift",
    "V2UnitV": "vector / float (add, min, shuffle)", "V2UnitL": "load", "V2UnitD": "store data",
    "V2UnitS": "integer ALU", "V2UnitM": "integer multi-cycle", "V2UnitB": "branch",
}
FLOAT_OPS = re.compile(r"\b(fadd|fminnm|fmin|fcmgt|fcmge|bif|bsl|bit|fcsel|fcmp)\b")


def hot_loop(asm):
    """Return the body of the innermost loop with the most float instructions."""
    lines = asm.splitlines()
    best, best_score, func = None, -1, ""
    for n, line in enumerate(lines):
        f = re.match(r"^(_[\w.$]+):", line)
        if f:
            func = f.group(1)
        if "Inner Loop Header" not in line or func == "_main":   # skip main()'s setup/checksum loops
            continue
        # The header comment can sit a few lines below its label.
        start = next(k for k in range(n, -1, -1) if re.match(r"^LBB\w+:", lines[k]))
        label, body = re.match(r"^(LBB\w+):", lines[start]).group(1), []
        for nxt in lines[start + 1:]:
            s = nxt.strip()
            if re.match(r"^b(\.\w+)?\s+" + label + r"\b", s) or re.match(r"^(cbn?z|tbn?z)\s.*\b" + label + r"\b", s):
                break
            if re.match(r"^_[\w.$]+:", s):   # ran into the next function: not a simple loop
                body = []
                break
            if not s or s.startswith((";", ".", "//")) or re.match(r"^L\w+:", s):
                continue
            body.append(s)
        score = sum(bool(FLOAT_OPS.search(b)) for b in body)
        if score > best_score:
            best, best_score = body, score
    return best


def run_mca(body, cpu):
    with tempfile.NamedTemporaryFile("w", suffix=".s", delete=False) as f:
        f.write("\n".join(body) + "\n")
        path = f.name
    try:
        p = subprocess.run([MCA, "-mtriple=arm64-apple-macos", f"-mcpu={cpu}", f"-iterations={ITER}",
                            "-instruction-info=false", path], capture_output=True, text=True)
        if p.returncode:
            sys.exit(p.stderr)
        return p.stdout
    finally:
        os.unlink(path)


def summarize(out):
    cycles = int(re.search(r"Total Cycles:\s+(\d+)", out).group(1)) / ITER
    names = dict(re.findall(r"^\[([\d.]+)\]\s+-\s+(\w+)", out, re.M))
    sec = out.split("Resource pressure per iteration:")[1].splitlines()
    cols = re.findall(r"\[([\d.]+)\]", sec[1])
    vals = sec[2].split()
    groups = defaultdict(lambda: [0, 0.0])
    for c, v in zip(cols, vals):
        g = re.sub(r"\d+$", "", names[c]) if names[c].startswith("V2Unit") else names[c]
        g = {"V2UnitL": "V2UnitL", "V2UnitL0": "V2UnitL"}.get(g, g)
        groups[g][0] += 1
        groups[g][1] += 0.0 if v == "-" else float(v)
    return cycles, groups


def main():
    src = sys.argv[1]
    asm = subprocess.run(CXX + [src], capture_output=True, text=True, check=True).stdout
    body = hot_loop(asm)
    if not body:
        sys.exit("no inner loop found")
    counts = defaultdict(int)
    for b in body:
        counts[b.split()[0]] += 1
    print(f"{src}: hot inner loop, {len(body)} instructions per iteration")
    print("  " + ", ".join(f"{k} x{v}" for k, v in sorted(counts.items(), key=lambda kv: -kv[1])))
    for cpu in MODELS:
        out = run_mca(body, cpu)
        cycles, groups = summarize(out)
        print(f"\n[{cpu} model]  ~{cycles:.1f} cycles per iteration")
        print(f"  {'pipe group':<12}{'pipes':>6}{'busy':>8}   {'':<20}  what it does")
        for g, (units, p) in sorted(groups.items(), key=lambda kv: -kv[1][1] / (kv[1][0] * cycles)):
            if p == 0:
                continue
            util = p / (units * cycles)
            bar = "#" * round(util * 20)
            print(f"  {g:<12}{units:>6}{util:>7.0%}   {bar:<20}  {DESCRIBE.get(g, '')}")
        if "--raw" in sys.argv:
            print(out)


if __name__ == "__main__":
    main()
