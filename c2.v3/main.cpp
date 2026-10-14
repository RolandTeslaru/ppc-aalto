#include <iostream>
#include <algorithm>
#include <limits>
#include <chrono>
#include <random>
#include <vector>

using namespace std;

typedef float float8_t __attribute__ ((vector_size (8 * sizeof(float))));

constexpr float infty = std::numeric_limits<float>::infinity();

constexpr float8_t f8infty {
    infty, infty, infty, infty,
    infty, infty, infty, infty
};

// horizontal minimum (each row each lane minimum)
static inline float hmin8(float8_t vv){
    float v = infty;

    for (int i = 0; i < 8; ++i) {
        v = min(vv[i], v);
    }
    return v;
}

static inline float8_t min8(float8_t x, float8_t y) {
#if defined(__aarch64__)
    return __builtin_elementwise_min(x, y); // ARM: one fminnm per half (ternary would be fcmgt + bif).
#else
    return x < y ? x : y; // x86: matches vminps exactly, one instruction.
#endif
}
// x:         [2, 8, 4, 1]
// y:         [3, 1, 4, 6]
// x < y:     [T, F, F, T]
// result:    [2, 1, 4, 1]

void step(float* r, const float* d_, int n){
    //  n is the original witdh, ex 9
    //  nab 
    // d_ is the unppadded old d

    constexpr int nb = 8;   // nr of items for a group, 8 items in a group
    int na = (n + nb - 1) / nb; // nr of groups, 3 groups with 8 items each

    // input data, padded, converted to vectors
    std::vector<float8_t> vd(n * na);
    // input data, transposed, padded, converted to vectors
    std::vector<float8_t> vt(n * na);


    // Create the padded and the transpose again
    // std::vector<float> t(n*n); // when we didnt use padding
    #pragma omp parallel for
    for (int row = 0; row < n; ++row) {
        for (int ka = 0; ka < na; ++ka) {
            for (int kb = 0; kb < nb; ++kb) {
                int col = ka * nb + kb;

                // Copy original d_[row][col] into this row's vector lane.
                vd[na*row + ka][kb] =               // vd[row][ka][kb]
                    col < n ? d_[n*row + col] : infty;

                // Copy original d_[col][row] into the transposed row.
                vt[na*row + ka][kb] =
                    col < n ? d_[n*col + row] : infty;
            }
        }
    }

    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            float8_t vv = f8infty;
            // For each horizontal group ka on a row i
            for (int ka = 0; ka< na; ++ka){
                // x = vd[i][ka]
                float8_t x = vd[na*i + ka];
                float8_t y = vt[na*j + ka];
                float8_t z = x + y;
                vv = min8(vv, z);
            }
            r[n*i + j] = hmin8(vv); // r[i][j] = min([...vv])
        }
    }
}




int main() {
    
  constexpr int n = 4000;
    // n is one integer. n*n = 16,000,000 elements per matrix.
    
    // Each vector owns contiguous float storage, initially [0,0,0,...].
    // Each has 16,000,000 floats (about 64 MB on this machine).
    vector<float> d(n*n); // Input: direct edge costs, stored row by row.
    vector<float> r(n*n); // Output: cheapest costs using at most two edges.

    // generator is an object holding the random algorithm's internal state.
    // 42 is its seed, not a generated value; the same seed repeats the sequence.
    mt19937 generator(42);
    // distribution is an object describing how to generate floats in [0,1).
    // It is not an array of random numbers; each call produces one float.
    uniform_real_distribution<float> distribution(0.0f, 1.0f);

    for (float& value : d){
        // value refers to an actual element: d[0], then d[1], then d[2],...
        // Write a new random float into that element (e.g. 0.37).
        value = distribution(generator);
    }

    for (int i = 0; i < n; ++i){
        // Replace diagonal costs with zero: staying at the same node is free.
        // For n=3 these positions are d[0], d[4], d[8].
        d[i*n + i] = 0.0f;
    }

    // start/end are time-point objects: readings from a monotonic clock.
    const auto start = chrono::steady_clock::now();

    // .data() returns a pointer to the vector's first float, without copying.
    // step reads d and fills the existing r storage.
    step(r.data(), d.data(), n);

    const auto end = chrono::steady_clock::now();
    // end-start is a duration; seconds is one double, e.g. 187.664.
    const double seconds = chrono::duration<double>(end - start).count();

    // checksum is one double accumulating the sum of all output floats.
    // It is a summary of r, not proof that every result is correct.
    double checksum = 0;

    for (float value : r) {
        // No &: value is a copy of the current float, since we only read it.
        checksum += value;
    }

    // One addition and one minimum per inner iteration: 2*n^3 operations.
    // For n=4000, operations is the double value 128,000,000,000.
    const double operations = 2.0 * n * n * n;

    std::cout << "Time: " << seconds << " seconds\n";
    std::cout << "Useful operations/second: "
              << operations / seconds / 1e9 << " billion\n";
    std::cout << "Checksum: " << checksum << '\n';

}
