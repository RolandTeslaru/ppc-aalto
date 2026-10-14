# Programming Parallel Computers: my work

My code while working through Aalto's [Programming Parallel Computers](https://ppc.cs.aalto.fi/) course, run and measured on an Apple M3 Pro (ARM / NEON) and a GTX 1060 (CUDA).

## Chapter 2: the shortcut problem, V0 to V7

`r[i][j] = min over k of d[i][k] + d[k][j]`, made faster one step at a time. Files are in [`versions/`](versions/).

| Version | Idea |
|---|---|
| V0 | Baseline triple loop |
| V1 | Transposed copy for sequential reads, plus OpenMP |
| V2 | 4 independent dependency chains (instruction-level parallelism) |
| V3 | 8-wide vectors (`float8_t`) |
| V4 | 3×3 block of results, reusing data in registers |
| V5 | 8×8 block via in-register lane permutes |
| V6 | Software prefetching |
| V7 | Z-order traversal of result blocks |

Measured on the M3 Pro, n = 4000:

| | Time |
|---|---|
| V4 | 0.60 s |
| V5 | 0.36 s (~355 billion useful ops/s, about 95% of the per-core vector-pipe limit) |
| V6, V7 | no further gain: V5 is already compute-bound on this machine |

Findings along the way:
- **`min8` as `x < y ? x : y` costs two instructions on ARM** (`fcmgt` + `bif`) but one on x86 (`vminps`). `__builtin_elementwise_min` compiles to a single `fminnm` on ARM and made V5 ~30% faster. The headers of v3–v7 use whichever form is fast on the target.
- [`tools/pipes.py`](tools/pipes.py) compiles a version, extracts its hot loop and runs `llvm-mca` to show per-pipe usage. LLVM's `apple-m3` model is really the 2013 Cyclone core (3 vector pipes), so the script also shows Neoverse V2 (4 × 128-bit pipes), which matches measured M3 timings.

Build any version (macOS, Homebrew `libomp`):

```bash
clang++ -std=c++17 -O3 -Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include \
  -L/opt/homebrew/opt/libomp/lib -lomp versions/v5.cpp -o v5 && ./v5
```

## GPU

The same "which block, thread, warp and lane am I" kernel, three ways:

- [`cuda/mykernel.cu`](cuda/mykernel.cu): CUDA, also prints the GPU's SM, register and shared-memory limits. `nvcc -O3 -arch=sm_61 mykernel.cu -o mykernel`
- [`gpu/mykernel.cpp`](gpu/mykernel.cpp) + [`gpu/mykernel.metal`](gpu/mykernel.metal): Metal from plain C++ via Apple's [metal-cpp](https://github.com/apple/metal-cpp) (vendored in `gpu/third_party/`, Apache-2.0).
- [`gpu/mykernel.mm`](gpu/mykernel.mm): the same host code in Objective-C++.

Graded course exercises are intentionally not included.
