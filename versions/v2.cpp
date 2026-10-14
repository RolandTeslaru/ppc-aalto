// V2: Adds four independent scalar minimum accumulators (ILP).
// Pads each input row to a multiple of four using infinity.
// Keeps transposed row reads and OpenMP. Not explicit SIMD vector code.
// Historical n=4000 time: 2.47347 s.
#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

constexpr float infty = std::numeric_limits<float>::infinity();

void step(float* r, const float* original, int n) {
    constexpr int nb = 4; // Positions per group / independent accumulators.
    const int na = (n+nb-1)/nb; // Number of groups, rounded up.
    const int nab = na*nb; // Padded row width in floats.
    std::vector<float> d(n*nab, infty), t(n*nab, infty);
    #pragma omp parallel for
    for (int row = 0; row < n; ++row) {
        for (int col = 0; col < n; ++col) {
            d[nab*row + col] = original[n*row + col]; // d[row][col]
            t[nab*col + row] = original[n*row + col]; // t[col][row]
        }
    }
    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            float vv[nb]; // Four scalar accumulators, initially all infinity.
            
            for (int lane = 0; lane < nb; ++lane) 
                vv[lane] = infty;
            
            for (int group = 0; group < na; ++group) {
                for (int lane = 0; lane < nb; ++lane) {
                    const int k = group*nb + lane;
                    float x = d[nab*i + k]; // Padded d[i][k]
                    float y = t[nab*j + k]; // Padded t[j][k] = original[k][j]
                    vv[lane] = std::min(vv[lane], x+y);
                }
            }
            float v = infty; // Combine the four partial minima.
            
            for (int lane = 0; lane < nb; ++lane) 
                v = std::min(v, vv[lane]);
            
            r[n*i + j] = v; // Output remains unpadded: r[i][j].
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
