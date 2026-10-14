// Metal translation of the CUDA example:
//
//   __device__ void foo(int i, int j) {}
//   __global__ void mykernel() { int i = blockIdx.x; int j = threadIdx.x; foo(i, j); }
//
// CUDA -> Metal names:
//   __global__ kernel          -> kernel
//   __device__ function        -> plain function
//   blockIdx.x                 -> [[threadgroup_position_in_grid]]
//   threadIdx.x                -> [[thread_position_in_threadgroup]]
//   warp index / lane          -> [[simdgroup_index_in_threadgroup]] / [[thread_index_in_simdgroup]]
#include <metal_stdlib>
using namespace metal;

// CUDA's foo was empty. Here it records what this thread saw, so the host can check
// that every thread ran and see how threads were grouped into SIMD-groups (warps).
static void foo(int i, int j, uint simd, uint lane, device int4* slot) {
    *slot = int4(i, j, int(simd), int(lane));
}

kernel void mykernel(device int4* out          [[buffer(0)]],
                     uint block                 [[threadgroup_position_in_grid]],
                     uint tid                   [[thread_position_in_threadgroup]],   // "thread" is reserved in Metal
                     uint threads_per_block     [[threads_per_threadgroup]],
                     uint simd                  [[simdgroup_index_in_threadgroup]],
                     uint lane                  [[thread_index_in_simdgroup]]) {
    int i = block;    // blockIdx.x
    int j = tid;      // threadIdx.x
    foo(i, j, simd, lane, &out[i * threads_per_block + j]);
}
