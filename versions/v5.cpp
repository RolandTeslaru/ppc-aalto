// V5: More register reuse.
// A vector now holds 8 different rows (or columns) at the SAME k, instead of 8 k's of one row.
// 2 vector loads per k + 4 lane permutes give all 8x8 = 64 (i, j) pairs: 8 accumulators x 8 lanes.
// Result (i, j) inside a block lands in accumulator number (i XOR j); unshuffled when writing r.
// Uses __builtin_shufflevector (Clang, GCC 12+). On M3 Pro a float8_t is two 128-bit NEON registers.
// n=4000 on M3 Pro: 0.52 s with ternary min8; 0.36 s with fminnm min8.
#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

constexpr float infty = std::numeric_limits<float>::infinity();

typedef float float8_t __attribute__((vector_size(8*sizeof(float))));
constexpr float8_t f8infty = {infty,infty,infty,infty,infty,infty,infty,infty};
static inline float8_t min8(float8_t x, float8_t y) {
#if defined(__aarch64__)
    return __builtin_elementwise_min(x, y); // ARM: one fminnm per half (ternary would be fcmgt + bif).
#else
    return x < y ? x : y; // x86: matches vminps exactly, one instruction.
#endif
}

// Lane permutes. Values stay the same, only their lane positions change.
// swap4: 0 1 2 3 4 5 6 7 -> 4 5 6 7 0 1 2 3   (swap halves)
// swap2: 0 1 2 3 4 5 6 7 -> 2 3 0 1 6 7 4 5   (swap pairs)
// swap1: 0 1 2 3 4 5 6 7 -> 1 0 3 2 5 4 7 6   (swap neighbours)
static inline float8_t swap4(float8_t x) { return __builtin_shufflevector(x, x, 4,5,6,7,0,1,2,3); }
static inline float8_t swap2(float8_t x) { return __builtin_shufflevector(x, x, 2,3,0,1,6,7,4,5); }
static inline float8_t swap1(float8_t x) { return __builtin_shufflevector(x, x, 1,0,3,2,5,4,7,6); }

void step(float* r, const float* d_, int n) {
    constexpr int nb = 8;         // Lanes per vector = rows (or columns) per block.
    int na = (n + nb - 1) / nb;   // Blocks of 8 rows needed, rounded up.

    // New layout: one vector = 8 rows at the same k.
    //   vd[n*ia + k][lane] = d[ia*8 + lane][k]   (piece of column k of d)
    //   vt[n*ja + k][lane] = d[k][ja*8 + lane]   (piece of row k of d)
    // For a fixed block ia, the vectors for k = 0, 1, 2, ... sit next to each
    // other in memory, so the k loop below reads linearly.
    std::vector<float8_t> vd(na * n);
    std::vector<float8_t> vt(na * n);

    #pragma omp parallel for
    for (int ja = 0; ja < na; ++ja) {
        for (int k = 0; k < n; ++k) {
            for (int lane = 0; lane < nb; ++lane) {
                int idx = ja*nb + lane;   // row (for vd) or column (for vt) of d
                vd[n*ja + k][lane] = idx < n ? d_[n*idx + k] : infty;
                vt[n*ja + k][lane] = idx < n ? d_[n*k + idx] : infty;
            }
        }
    }

    #pragma omp parallel for
    for (int ia = 0; ia < na; ++ia) {
        for (int ja = 0; ja < na; ++ja) {
            // vv[s] with s = XYZ in binary: a-permute XY (swap4, swap2), b-permute Z (swap1).
            float8_t vv[8];
            for (int s = 0; s < 8; ++s) vv[s] = f8infty;

            for (int k = 0; k < n; ++k) {
                float8_t a000 = vd[n*ia + k];   // rows    i0 .. i7 at k
                float8_t b000 = vt[n*ja + k];   // columns j0 .. j7 at k
                float8_t a100 = swap4(a000);    // i4 i5 i6 i7 i0 i1 i2 i3
                float8_t a010 = swap2(a000);    // i2 i3 i0 i1 i6 i7 i4 i5
                float8_t a110 = swap2(a100);    // i6 i7 i4 i5 i2 i3 i0 i1
                float8_t b001 = swap1(b000);    // j1 j0 j3 j2 j5 j4 j7 j6
                vv[0b000] = min8(vv[0b000], a000 + b000);
                vv[0b001] = min8(vv[0b001], a000 + b001);
                vv[0b010] = min8(vv[0b010], a010 + b000);
                vv[0b011] = min8(vv[0b011], a010 + b001);
                vv[0b100] = min8(vv[0b100], a100 + b000);
                vv[0b101] = min8(vv[0b101], a100 + b001);
                vv[0b110] = min8(vv[0b110], a110 + b000);
                vv[0b111] = min8(vv[0b111], a110 + b001);
            }

            // Unshuffle. Lane l of vv[s] paired a-lane (l ^ (s & 6)) with b-lane (l ^ (s & 1)),
            // so it holds r[ia*8 + (l ^ (s & 6))][ja*8 + (l ^ (s & 1))].
            for (int s = 0; s < 8; ++s) {
                for (int lane = 0; lane < nb; ++lane) {
                    int i = ia*nb + (lane ^ (s & 6));
                    int j = ja*nb + (lane ^ (s & 1));
                    if (i < n && j < n) {
                        r[n*i + j] = vv[s][lane];
                    }
                }
            }
        }
    }
}

int main() {
    constexpr int n = 4000; // Change to a smaller value for a quick run.
    std::vector<float> d(n*n), r(n*n);
    std::mt19937 generator(42); // Same input sequence for every version.
    std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    for (float& value : d) value = distribution(generator);
    for (int i = 0; i < n; ++i) d[n*i + i] = 0.0f;

    // Time all of step, including any transpose, padding and allocation.
    const auto start = std::chrono::steady_clock::now();
    step(r.data(), d.data(), n);
    const auto end = std::chrono::steady_clock::now();
    const double seconds = std::chrono::duration<double>(end-start).count();
    double checksum = 0;
    for (float value : r) checksum += value;
    const double operations = 2.0*n*n*n; // Useful add/min work, not all instructions.
    std::cout << "Time: " << seconds << " seconds\n"
              << "Useful operations/second: " << operations/seconds/1e9 << " billion\n"
              << "Checksum: " << checksum << '\n';
    return 0;
}
