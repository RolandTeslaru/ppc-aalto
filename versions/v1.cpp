// V1: Adds a transposed copy so BOTH inner-loop reads are consecutive.
// Keeps V0's one running minimum and OpenMP.
// Historical n=4000 time: 4.93937 s.
#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

constexpr float infty = std::numeric_limits<float>::infinity();

void step(float* r, const float* d, int n) {
    std::vector<float> t(n*n);
    #pragma omp parallel for
    for (int row = 0; row < n; ++row) {
        for (int col = 0; col < n; ++col) {
            t[n*col + row] = d[n*row + col]; // t[col][row] = d[row][col]
        }
    }
    // The previous parallel loop completes before we read t.
    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            float v = infty;
            for (int k = 0; k < n; ++k) {
                float x = d[n*i + k]; // d[i][k]
                float y = t[n*j + k]; // t[j][k] = d[k][j]
                v = std::min(v, x+y);
            }
            r[n*i + j] = v; // r[i][j]
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
