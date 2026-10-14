// V7: Cache blocking with a Z-order curve.
// Same kernel as V6. Only the ORDER of the (ia, ja) result blocks changes:
// instead of row by row, blocks are visited along a Z-order (Morton) curve.
// Consecutive blocks then form compact squares that share vd/vt panels, so a panel
// loaded into cache serves many blocks before it is evicted (at every cache level).
// Uses __builtin_shufflevector (Clang, GCC 12+). On M3 Pro a float8_t is two 128-bit NEON registers.
// M3 Pro: n=4000 0.40 s (V6: 0.39 s); n=8000 1 thread 19.6 s (V6: 19.8 s). No gain: the V5 kernel
// does 64 add+min per 64 bytes loaded, so even row-by-row order keeps it compute-bound.
#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <random>
#include <tuple>
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

// Spread the low 16 bits of x apart: b15..b1 b0 -> 0 b15 .. 0 b1 0 b0.
// (x86 has _pdep_u32 for this; ARM has no single instruction, so use shifts and masks.)
static inline unsigned spread_bits(unsigned x) {
    x &= 0xFFFF;
    x = (x | (x << 8)) & 0x00FF00FF;
    x = (x | (x << 4)) & 0x0F0F0F0F;
    x = (x | (x << 2)) & 0x33333333;
    x = (x | (x << 1)) & 0x55555555;
    return x;
}

// Z-order key: interleave the bits of ia (even positions) and ja (odd positions).
//   ia = 0b101, ja = 0b011  ->  j2 i2 j1 i1 j0 i0 = 0b011011
static inline unsigned zorder(unsigned ia, unsigned ja) {
    return spread_bits(ia) | (spread_bits(ja) << 1);
}

void step(float* r, const float* d_, int n) {
    constexpr int nb = 8;         // Lanes per vector = rows (or columns) per block.
    int na = (n + nb - 1) / nb;   // Blocks of 8 rows needed, rounded up.

    // New layout: one vector = 8 rows at the same k.
    //   vd[n*ia + k][lane] = d[ia*8 + lane][k]   (piece of column k of d)
    //   vt[n*ja + k][lane] = d[k][ja*8 + lane]   (piece of row k of d)
    // For a fixed block ia, the vectors for k = 0, 1, 2, ... sit next to each
    // other in memory, so the k loop below reads linearly.
    constexpr int PF = 20;        // Prefetch distance in k iterations.
    std::vector<float8_t> vd(na * n + PF);   // + PF: room for the last block's prefetches.
    std::vector<float8_t> vt(na * n + PF);

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

    // List every result block with its Z-order key, then sort by the key.
    //   row by row:  (0,0) (0,1) (0,2) (0,3) ...    Z-order:  0  1 |  4  5
    //                (1,0) (1,1) (1,2) (1,3) ...              2  3 |  6  7
    //                                                         ------+------
    //                                                         8  9 | 12 13
    //                                                        10 11 | 14 15
    std::vector<std::tuple<unsigned, int, int>> blocks(na * na);
    for (int ia = 0; ia < na; ++ia) {
        for (int ja = 0; ja < na; ++ja) {
            blocks[na*ia + ja] = {zorder(ia, ja), ia, ja};
        }
    }
    std::sort(blocks.begin(), blocks.end());

    // Static schedule: each thread gets one contiguous run of the curve, which is a
    // compact region of blocks, so threads sharing the L2 reuse each other's panels.
    #pragma omp parallel for schedule(static)
    for (int b = 0; b < na * na; ++b) {
        const int ia = std::get<1>(blocks[b]);
        const int ja = std::get<2>(blocks[b]);

        // vv[s] with s = XYZ in binary: a-permute XY (swap4, swap2), b-permute Z (swap1).
        float8_t vv[8];
        for (int s = 0; s < 8; ++s) vv[s] = f8infty;

        for (int k = 0; k < n; ++k) {
            __builtin_prefetch(&vd[n*ia + k + PF]);   // ask for data needed PF steps from now
            __builtin_prefetch(&vt[n*ja + k + PF]);
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
