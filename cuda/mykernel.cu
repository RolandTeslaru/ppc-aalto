// The course's first CUDA example, made observable: foo() records what each thread saw.
// Same experiment as ../gpu/mykernel.metal + mykernel.cpp, so the two can be compared.
//
// Build and run on the Linux machine (GTX 1060 = Pascal, compute capability 6.1):
//   nvcc -O3 -arch=sm_61 mykernel.cu -o mykernel && ./mykernel
#include <cstdio>
#include <cstdlib>
#include <cuda_runtime.h>

// Stop with a readable message if a CUDA call fails.
#define CHECK(call)                                                                     \
    do {                                                                                \
        cudaError_t err_ = (call);                                                      \
        if (err_ != cudaSuccess) {                                                      \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(err_)); \
            exit(1);                                                                    \
        }                                                                               \
    } while (0)

__device__ void foo(int i, int j, int4* slot) {
    int warp = j / warpSize;   // which warp of the block this thread is in
    int lane = j % warpSize;   // which lane of that warp
    *slot = make_int4(i, j, warp, lane);
}

__global__ void mykernel(int4* out) {
    int i = blockIdx.x;
    int j = threadIdx.x;
    foo(i, j, &out[i * blockDim.x + j]);
}

int main() {
    constexpr int blocks = 100, threads = 128;

    cudaDeviceProp p;
    CHECK(cudaGetDeviceProperties(&p, 0));
    printf("GPU: %s (compute capability %d.%d)\n", p.name, p.major, p.minor);
    printf("SMs: %d, warp size: %d, max threads per block: %d\n",
           p.multiProcessorCount, p.warpSize, p.maxThreadsPerBlock);
    printf("registers per SM: %d, shared memory per SM: %zu KB, L2: %d KB\n",
           p.regsPerMultiprocessor, p.sharedMemPerMultiprocessor / 1024, p.l2CacheSize / 1024);
    printf("max threads resident per SM: %d (= %d warps)\n\n",
           p.maxThreadsPerMultiProcessor, p.maxThreadsPerMultiProcessor / p.warpSize);

    // GPU memory is separate from CPU memory here (unlike the Mac's unified memory),
    // so the results are copied back explicitly.
    int4* d_out = nullptr;
    CHECK(cudaMalloc(&d_out, blocks * threads * sizeof(int4)));

    mykernel<<<blocks, threads>>>(d_out);
    CHECK(cudaGetLastError());         // launch errors (e.g. wrong -arch)
    CHECK(cudaDeviceSynchronize());    // errors while the kernel ran

    int4* out = (int4*)malloc(blocks * threads * sizeof(int4));
    CHECK(cudaMemcpy(out, d_out, blocks * threads * sizeof(int4), cudaMemcpyDeviceToHost));

    int bad = 0;
    for (int i = 0; i < blocks; ++i)
        for (int j = 0; j < threads; ++j)
            if (out[i * threads + j].x != i || out[i * threads + j].y != j) ++bad;
    printf("%d threads ran, %d wrong\n\n", blocks * threads, bad);

    printf("block  thread  ->  warp  lane\n");
    for (int i : {0, 99})
        for (int j : {0, 1, 31, 32, 33, 63, 64, 127}) {
            int4 t = out[i * threads + j];
            printf("%5d  %6d  ->  %4d  %4d\n", t.x, t.y, t.z, t.w);
        }

    free(out);
    CHECK(cudaFree(d_out));
    return 0;
}
