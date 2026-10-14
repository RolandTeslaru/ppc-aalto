#include <iostream>
#include <algorithm>
#include <limits>
#include <chrono>
#include <random>
#include <vector>


using namespace std;

constexpr float infty = std::numeric_limits<float>::infinity();
    
void step(float* r, const float* d_, int n){
    //  n is the original witdh, ex 9
    //  nab 
    // d_ is the unppadded old d

    constexpr int nb = 4;   // nr of items for a group, 4 items in a group
    int na = (n + nb - 1) / nb; // nr of groups, 3 groups with 4 items each
    int nab = na*nb;    // nab is the padded with of n so that it can form groups of 4 items, for n = 9, nab = 12

    std::vector<float> d(n*nab, infty);
    std::vector<float> t(n*nab, infty);


    // std::vector<float> t(n*n); // when we didnt use padding
    #pragma omp parallel for
    for (int i = 0; i<n; ++i){
        for (int j =0; j<n; ++j){
            // t[j*n + i] = d[i*n + j];
            d[nab*j + i] = d_[n*j + i];
            t[nab*j + i] = d_[n*i + j];
        }
    }

    #pragma omp parallel for
    for (int i = 0; i < n; ++i){
        for (int j=0; j<n; ++j){

            // float v = numeric_limits<float>::infinity();
            
            float vv[nb]; // nb is 4 as in 4 items in a group, 
            // becaseu were going to parallize the min isntruction , so were going to the 4 min operaitons in parallel.
            // therefore vv is where the result are going to be.

            // init it
            // compiler sees nb is constant 4, so the loop wont add overhead because it will be unrolled. 
            for (int kb = 0; kb < nb; ++kb){
                vv[kb] = infty;
            }
            // The compiler turns it into this
            // vv[0] = infty;
            // vv[1] = infty;
            // vv[2] = infty;
            // vv[3] = infty;



            for (int ka = 0; ka < na; ++ka) {
                // this is the group parallelism loop, nb = 4 thus 4 
                for (int kb = 0; kb < nb; ++kb) {
                    float x = d[nab*i + ka * nb + kb];
                    float y = t[nab*j + ka * nb + kb];
                    float z = x + y;
                    vv[kb] = min(vv[kb], z);
                }
            }

            // fin the min of the 3 groups (na)
            float v = infty;
            for (int kb = 0; kb<nb; ++kb) {
                v = min(vv[kb], v);
            }

            // without instruction parallelism
            // // k tries every intermediate node: 0,1,...,n-1.
            // // Example with i=0,j=1,k=2 in the example matrix above:
            // for (int k=0; k<n; ++k){
            //     // x = cost i->k; example: d[0*3+2] = d[2] = 2.
            //     float x = d[n*i + k]; // d[i][k]
            //     // y = cost k->j; example: d[2*3+1] = d[7] = 5.
            //     // float y = d[n*k + j]; // d[k][j]
            //     float y = t[n*j + k];
            //     // z = candidate route cost i->k->j; example: 2+5 = 7.
            //     float z = x + y;
            //     // Keep the smaller float: previous best or this candidate.
            //     v = std::min(v, z);
            // }

            // Store one result; example: r[0*3+1] = r[1] = 7.
            r[i*n + j] = v;
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
