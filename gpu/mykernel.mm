// Host side of the CUDA example, in Metal (Objective-C++).
//
//   CUDA                              Metal
//   mykernel<<<100, 128>>>();    ->   dispatchThreadgroups: 100 threadgroups of 128 threads
//   cudaDeviceSynchronize();     ->   [commandBuffer waitUntilCompleted]
//
// Build and run (from this folder):
//   clang++ -std=c++17 -fobjc-arc -framework Metal -framework Foundation mykernel.mm -o mykernel && ./mykernel
#import <Metal/Metal.h>
#include <cstdio>
#include <string>

int main() {
    @autoreleasepool {
        constexpr int blocks = 100, threads = 128;

        id<MTLDevice> device = MTLCreateSystemDefaultDevice();   // the GPU

        // Compile mykernel.metal (next to this file) at runtime.
        std::string dir = __FILE__;
        dir = dir.substr(0, dir.find_last_of('/') + 1);
        NSError* error = nil;
        NSString* source = [NSString stringWithContentsOfFile:@((dir + "mykernel.metal").c_str())
                                                     encoding:NSUTF8StringEncoding error:&error];
        if (!source) { printf("cannot read mykernel.metal: %s\n", error.localizedDescription.UTF8String); return 1; }
        id<MTLLibrary> library = [device newLibraryWithSource:source options:nil error:&error];
        if (!library) { printf("compile error: %s\n", error.localizedDescription.UTF8String); return 1; }
        id<MTLComputePipelineState> pipeline =
            [device newComputePipelineStateWithFunction:[library newFunctionWithName:@"mykernel"] error:&error];

        // One int4 per thread. Shared storage: CPU and GPU see the same memory, no copies.
        id<MTLBuffer> out = [device newBufferWithLength:blocks * threads * sizeof(int) * 4
                                                options:MTLResourceStorageModeShared];

        // mykernel<<<100, 128>>>();
        id<MTLCommandQueue> queue = [device newCommandQueue];
        id<MTLCommandBuffer> commands = [queue commandBuffer];
        id<MTLComputeCommandEncoder> encoder = [commands computeCommandEncoder];
        [encoder setComputePipelineState:pipeline];
        [encoder setBuffer:out offset:0 atIndex:0];
        [encoder dispatchThreadgroups:MTLSizeMake(blocks, 1, 1)
                threadsPerThreadgroup:MTLSizeMake(threads, 1, 1)];
        [encoder endEncoding];
        [commands commit];

        // cudaDeviceSynchronize();
        [commands waitUntilCompleted];

        printf("GPU: %s\n", device.name.UTF8String);
        printf("SIMD-group (warp) width: %lu threads\n", (unsigned long)pipeline.threadExecutionWidth);
        printf("max threads per threadgroup (block): %lu\n\n", (unsigned long)pipeline.maxTotalThreadsPerThreadgroup);

        // Check that every (block, thread) ran exactly where expected.
        const int* r = (const int*)out.contents;
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
    }
    return 0;
}
