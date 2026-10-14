// Host side of the CUDA example, in C++ with Apple's metal-cpp.
//
//   CUDA                              Metal
//   mykernel<<<100, 128>>>();    ->   dispatchThreadgroups: 100 threadgroups of 128 threads
//   cudaDeviceSynchronize();     ->   commandBuffer->waitUntilCompleted()
//
// Build and run (from this folder):
//   clang++ -std=c++17 -O2 -I third_party/metal-cpp mykernel.cpp -framework Metal -framework Foundation -o mykernel && ./mykernel

// metal-cpp is header-only: exactly one .cpp file must define these to emit its implementation.
#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

static NS::String* ns(const char* s) { return NS::String::string(s, NS::UTF8StringEncoding); }

int main() {
    // metal-cpp follows Objective-C memory rules: objects from new...() must be release()d,
    // other returned objects are cleaned up when this pool is released.
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
    constexpr int blocks = 100, threads = 128;

    MTL::Device* device = MTL::CreateSystemDefaultDevice();   // the GPU

    // Compile mykernel.metal (next to this file) at runtime.
    std::string dir = __FILE__;
    dir = dir.substr(0, dir.find_last_of('/') + 1);
    std::ifstream file(dir + "mykernel.metal");
    if (!file) { printf("cannot read %smykernel.metal\n", dir.c_str()); return 1; }
    std::stringstream source;
    source << file.rdbuf();

    NS::Error* error = nullptr;
    MTL::Library* library = device->newLibrary(ns(source.str().c_str()), nullptr, &error);
    if (!library) { printf("compile error: %s\n", error->localizedDescription()->utf8String()); return 1; }
    MTL::Function* function = library->newFunction(ns("mykernel"));
    MTL::ComputePipelineState* pipeline = device->newComputePipelineState(function, &error);
    if (!pipeline) { printf("pipeline error: %s\n", error->localizedDescription()->utf8String()); return 1; }

    // One int4 per thread. Shared storage: CPU and GPU see the same memory, no copies.
    MTL::Buffer* out = device->newBuffer(blocks * threads * sizeof(int) * 4, MTL::ResourceStorageModeShared);

    // mykernel<<<100, 128>>>();
    MTL::CommandQueue* queue = device->newCommandQueue();
    MTL::CommandBuffer* commands = queue->commandBuffer();
    MTL::ComputeCommandEncoder* encoder = commands->computeCommandEncoder();
    encoder->setComputePipelineState(pipeline);
    encoder->setBuffer(out, 0, 0);                         // slot 0 = [[buffer(0)]] in the kernel
    encoder->dispatchThreadgroups(MTL::Size(blocks, 1, 1), MTL::Size(threads, 1, 1));
    encoder->endEncoding();
    commands->commit();

    // cudaDeviceSynchronize();
    commands->waitUntilCompleted();

    printf("GPU: %s\n", device->name()->utf8String());
    printf("SIMD-group (warp) width: %lu threads\n", (unsigned long)pipeline->threadExecutionWidth());
    printf("max threads per threadgroup (block): %lu\n\n", (unsigned long)pipeline->maxTotalThreadsPerThreadgroup());

    // Check that every (block, thread) ran exactly where expected.
    const int* r = static_cast<const int*>(out->contents());
    int bad = 0;
    for (int i = 0; i < blocks; ++i)
        for (int j = 0; j < threads; ++j) {
            const int* t = r + 4 * (i * threads + j);
            if (t[0] != i || t[1] != j) ++bad;
        }
    printf("%d threads ran, %d wrong\n\n", blocks * threads, bad);

    printf("block  thread  ->  simd-group  lane\n");
    for (int i : {0, 99})
        for (int j : {0, 1, 31, 32, 33, 63, 64, 127}) {
            const int* t = r + 4 * (i * threads + j);
            printf("%5d  %6d  ->  %10d  %4d\n", t[0], t[1], t[2], t[3]);
        }

    out->release();
    queue->release();
    pipeline->release();
    function->release();
    library->release();
    device->release();
    pool->release();
    return 0;
}
