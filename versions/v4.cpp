// V4:
// Simmilar to v2 but parallizes and loads the vectors in parallel 
// n=4000 on M3 Pro: 0.71-0.79 s with ternary min8; 0.60 s with fminnm min8.
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

void step(float* r, const float* d_, int n){

    // Split into groups and handle padding creating

    constexpr int nb = 8;         // Floats per SIMD vector: 8 lanes.
    int na = (n + nb - 1) / nb;   // Vectors needed per row, rounded up.

    constexpr int nd = 3;         // Output block size: calculate 3×3 results together.
    int nc = (n + nd - 1) / nd;   // Blocks of 3 rows needed, rounded up.
    int ncd = nc * nd;            // Padded row count: a multiple of 3.
    
    
    std::vector<float8_t> vd(ncd * na);
    // input data, transposed, padded, converted to vectors
    std::vector<float8_t> vt(ncd * na);

    #pragma omp parallel for
    for (int row = 0; row < n; ++row){
        for (int group = 0; group < na; ++group){ // group = ka
            for (int lane = 0; lane < nb; ++lane) {
                int col = group*nb + lane;
                // vd[row][group][col] = d[row][col]
                vd[row*na + group][lane] = col < n ? d_[n*row + col] : infty;
                // transpose
                // vt[row][group][col] =  d[col][row]
                vt[row*na + group][lane] = col < n ? d_[n*col + row] : infty;
            }
        }
    }

    // No omp parallel
    for (int row = n; row < ncd; ++row) {
        for (int group = 0; group < na; ++group) {
            for (int lane = 0; lane < nb; ++lane) {
                // vd[row][group][col] = infty
                // vt[row][group][col] = infty
                vd[row * na + group][lane] = infty;
                vt[row * na + group][lane] = infty;
            }
        }
    }


//                col_block = 0              col_block = 1
//            columns 0–2         columns 3–5

//          ┌─────────────────┬─────────────────┐
// row_block = 0   │ r00  r01  r02    │ r03   ·    ·    │
// rows 0–2 │ r10  r11  r12    │ r13   ·    ·    │
//          │ r20  r21  r22    │ r23   ·    ·    │
//          ├─────────────────┼─────────────────┤
// row_block = 1   │ r30  r31  r32    │ r33   ·    ·    │
// rows 3–5 │  ·    ·    ·     │  ·    ·    ·    │
//          │  ·    ·    ·     │  ·    ·    ·    │
//          └─────────────────┴─────────────────┘

// · = padding position, not a real output


// nc is the number of blocks per row / column

//                     col_block
//                   0     1     2
//                ┌─────┬─────┬─────┐
// row_block  0   │ 3×3 │ 3×3 │ 3×3 │
//                ├─────┼─────┼─────┤
//            1   │ 3×3 │ 3×3 │ 3×3 │
//                ├─────┼─────┼─────┤
//            2   │ 3×3 │ 3×3 │ 3×3 │
//                └─────┴─────┴─────┘

// nd the number of rows and columns inside each block

    // nd = 3
    // nc = 7 (number of blocks vertically)
    // 

    #pragma omp parallel for
    for (int row_block = 0; row_block < nc; ++row_block) {
        for (int col_block = 0; col_block < nc; ++col_block) {
            float8_t vv[nd][nd]; // 3 x 3 accumulator

            // fill in the surface (3 x 3 of groups)
            for (int id = 0; id < nd; ++id){
                for (int jd = 0; jd < nd; ++jd){
                    vv[id][jd] = f8infty;
                }
            }

            for (int group = 0; group < na; ++group) {
                // y0 = vt[]
                float8_t y0 = vt[na*(col_block * nd + 0) + group];
                float8_t y1 = vt[na*(col_block * nd + 1) + group];
                float8_t y2 = vt[na*(col_block * nd + 2) + group];
                float8_t x0 = vd[na*(row_block * nd + 0) + group];
                float8_t x1 = vd[na*(row_block * nd + 1) + group];
                float8_t x2 = vd[na*(row_block * nd + 2) + group];
                vv[0][0] = min8(vv[0][0], x0 + y0);
                vv[0][1] = min8(vv[0][1], x0 + y1);
                vv[0][2] = min8(vv[0][2], x0 + y2);
                vv[1][0] = min8(vv[1][0], x1 + y0);
                vv[1][1] = min8(vv[1][1], x1 + y1);
                vv[1][2] = min8(vv[1][2], x1 + y2);
                vv[2][0] = min8(vv[2][0], x2 + y0);
                vv[2][1] = min8(vv[2][1], x2 + y1);
                vv[2][2] = min8(vv[2][2], x2 + y2);
            }
            for (int id = 0; id < nd; ++id) {
                for (int jd = 0; jd < nd; ++jd) {
                    int i = row_block * nd + id;
                    int j = col_block * nd + jd;
                    if (i < n && j < n) {
                        r[n*i + j] = hmin8(vv[id][jd]);
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
