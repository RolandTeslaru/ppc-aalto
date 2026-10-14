// V3: Adds explicit SIMD with eight-float compiler vector types.
// Packed addition/minimum updates eight lanes, then horizontal minimum combines them.
// Keeps padding, transpose and OpenMP on BOTH preparation and calculation.
// On M3 Pro an eight-float value spans two 128-bit hardware vectors.
// Historical n=4000 time: 1.57781 s. GCC/Clang extension; compile as C++17.
// With fminnm min8 on M3 Pro: 1.52 s (was 1.55 s same day): memory-bound, so faster min barely helps.
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


static inline float hmin8(float8_t values) {
    float v = infty;
    for (int lane = 0; lane < 8; ++lane) v = std::min(v, values[lane]);
    return v; // Horizontal minimum: eight lanes -> one float.
}


void step(float* r, const float* original, int n) {
    constexpr int nb = 8;
    const int na = (n+nb-1)/nb; // Groups per row (n=9 -> 2 groups).

    std::vector<float8_t> vd(n*na), vt(n*na);

    #pragma omp parallel for
    for (int row = 0; row < n; ++row) {
        for (int group = 0; group < na; ++group) {
            for (int lane = 0; lane < nb; ++lane) {
                const int col = group*nb + lane;
                vd[na*row + group][lane] = col < n ? original[n*row + col] : infty;
                vt[na*row + group][lane] = col < n ? original[n*col + row] : infty;
                // vd[row][group][lane] copies original[row][col].
                // vt[row][group][lane] copies original[col][row].
            }
        }
    }
    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            // ARM assembly comments use //; x86 examples often use #.
            // These markers emit no CPU instructions. Optimized instructions
            // may move around them, so they are navigation aids, not boundaries.
            asm volatile("// SIMD: initialize eight lane accumulators to infinity");
            float8_t vv = f8infty; // Eight lane accumulators.
            for (int group = 0; group < na; ++group) {
                asm volatile("// SIMD: load eight costs from input row i and transposed row j");
                float8_t x = vd[na*i + group]; // Eight costs from row i.
                float8_t y = vt[na*j + group]; // Eight costs from original column j.
                asm volatile("// SIMD: add corresponding lanes and update eight running minima");
                vv = min8(vv, x+y); // Vector addition and elementwise minimum.
            }
            asm volatile("// SIMD: horizontal minimum combines eight lanes into one result");
            r[n*i + j] = hmin8(vv); // r[i][j]
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
