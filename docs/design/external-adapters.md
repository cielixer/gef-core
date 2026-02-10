# External Adapters Architecture

## Overview

GEF (General Engine Framework) is designed to be a "glue" framework that seamlessly integrates with established high-performance libraries like Eigen, OpenCV, and ArrayFire. Rather than reinventing linear algebra or computer vision primitives, GEF provides an **open adapter pattern**. This pattern allows researchers to use their preferred library's native types (e.g., `Eigen::Matrix`, `cv::Mat`) directly within GEF modules while the framework manages the underlying memory, concurrency, and resource lifetimes.

The adapter acts as a bridge between the `ResourcePool`-managed memory handles and the library-specific object models.

## Adapter Responsibilities

External adapters are responsible for the following:
- **Type Conversion**: Mapping GEF resource handles to library-native types.
- **Metadata Translation**: Converting GEF `Descriptor` information (dimensions, `dtype`, `stride`, `alignment`) into the format expected by the external library.
- **Safety Enforcement**: Ensuring that library-native objects respect the borrow rules of the GEF `ResourcePool`. The adapter must manage the lock acquisition and release lifecycle.
- **Performance Optimization**: Facilitating zero-copy views when memory layouts are compatible.
- **Fallback Management**: Handling cases where memory must be copied to meet the library's requirements.
- **API Ergonomics**: Providing a researcher-friendly C++ interface that makes external library integration feel like a natural extension of GEF.

## Zero-Copy View Mechanism

The primary goal of GEF's integration strategy is **Zero-Copy Performance**. When the memory layout managed by the `ResourcePool` is perfectly compatible with an external library's requirements, the adapter creates a direct "view" or "map" over the existing buffer.

Zero-copy is possible when:
1. **Alignment**: The start address of the buffer meets the library's SIMD or hardware alignment requirements (e.g., AVX-512 alignment).
2. **Stride**: The memory offsets between elements (pitch/stride) match the library's internal representation. For example, Eigen can handle non-unit strides using its `InnerStride` and `OuterStride` parameters.
3. **Layout**: The data organization (e.g., Row-major vs. Column-major, SoA vs. AoS) is identical to what the library expects.

When these conditions are met, the adapter returns a native object that points directly to the `ResourcePool` memory. This results in maximum performance with zero allocation or data movement overhead. The lifetime of this view is strictly tied to the duration of the module's `execute()` function, ensuring that the `ResourcePool` borrow rules are never violated.

## Copy Fallback

In scenarios where zero-copy is impossible—such as when a library requires a specific alignment that the original resource lacks, or when a module needs a contiguous block but the source is a non-contiguous sub-region—the adapter performs an **explicit copy**.

- **Automatic Fallback**: The adapter handles the allocation of a temporary compatible buffer and copies the data into it.
- **Profiling Logs**: Every time an explicit copy is performed by an adapter, GEF generates a profiling log entry. These entries include:
    - The module name triggering the copy.
    - The specific resource name and handle.
    - The reason for the copy (e.g., "Alignment mismatch", "Non-contiguous stride").
    - The size of the data and the time taken for the copy.
- **Optimization Visibility**: These logs are critical for Engineers. If a performance-critical module is frequently triggering fallback copies, the Engineer can adjust the `Descriptor` in the YAML configuration or modify the upstream module's output settings to ensure the memory is created in a compatible format from the start.

## Eigen Adapter

The Eigen adapter is the primary example of this pattern in GEF. It bridges GEF `Tensor` resources with `Eigen::Map`.

### How it Works
Eigen provides the `Eigen::Map` class, which allows the library to treat a raw pointer as a matrix or vector without owning the memory. The GEF Eigen adapter calculates the correct template parameters for `Eigen::Map` based on the resource's `dtype` and `stride`.

### Zero-Copy vs. Copy
- **Zero-Copy**: If the GEF resource is contiguous and matches the requested alignment (e.g., 16-byte alignment for SSE), the adapter creates an `Eigen::Map` pointing to the raw `ResourcePool` pointer.
- **Copy**: If the resource is non-contiguous or misaligned, the adapter allocates a temporary contiguous Eigen matrix, copies the data, and returns the map to that temporary buffer.

### Conceptual C++ Usage
```cpp
GEF_STAGE(LinearSolver) {
    GEF_INPUT(Tensor<float, 2>, matrix_a);
    GEF_INPUT(Tensor<float, 1>, vector_b);
    GEF_OUTPUT(Tensor<float, 1>, solution_x);

    void execute(Context& ctx) {
        // Use the Eigen adapter to create views from GEF handles
        // gef::eigen::map detects if zero-copy is possible
        auto A = gef::eigen::map(ctx.input(matrix_a));
        auto b = gef::eigen::map(ctx.input(vector_b));
        auto x = gef::eigen::map(ctx.output(solution_x));

        // Perform native Eigen operations directly on ResourcePool memory
        // No data is copied if alignment/stride are compatible
        x = A.colPivHouseholderQr().solve(b);
        
        // When execute() returns, the views are destroyed and the 
        // ResourcePool handles are naturally released.
    }
}
```

## Memory Layout Handling

Adapters must intelligently handle the differences in memory organization between GEF and external libraries.

- **SoA vs. AoS**: GEF often uses Structure of Arrays (SoA) for better vectorization, while libraries like OpenCV typically prefer Array of Structures (AoS) for image data. The adapter identifies these differences and, if possible, provides a view that reinterprets the layout (e.g., using Eigen's `Stride` parameters) or triggers a layout-transforming copy.
- **Row-Major vs. Column-Major**: While GEF defaults to Row-Major, adapters can detect if a library (like Eigen or BLAS) prefers Column-Major and handle the mapping or transposition during the view creation phase.
- **Sub-region Mapping**: If a module is working on a ROI (Region of Interest) of a larger tensor, the adapter must correctly calculate the base pointer offset and the stride parameters to provide a valid native view of just that sub-region.

## Future Adapters

While the Eigen adapter is the first implementation, the architecture is designed to support:
- **OpenCV Adapter**: Mapping GEF `Image` resources to `cv::Mat` views. This will include support for both standard interleaved (AoS) and planar (SoA) images.
- **ArrayFire Adapter**: Integrating GEF tensors with `af::array` for high-level GPGPU operations.
- **Native GPU Libraries**: Providing adapters for libraries like NVIDIA cuDNN or NPP, where the adapter manages device pointers and stream state.

## GPU Memory Adapters

Future iterations of GEF will extend the adapter pattern to handle device-side memory. This is a critical area for performance in video processing and machine learning workflows.

- **Device Pointers**: Adapters will manage raw CUDA/OpenCL pointers instead of host pointers.
- **Synchronization**: Handlers for stream synchronization and memory fences to ensure data is ready before the external library accesses it.
- **Host/Device Transfers**: Standardized hooks for triggering DMA transfers when an external library requires data on a different backend than where it currently resides.
- **Inter-library GPU Sharing**: Facilitating zero-copy sharing between, for example, a CUDA kernel and an OpenGL texture via the adapter layer.

## Guidelines for Custom Adapters

Engineers can implement custom adapters for specialized internal or 3rd-party libraries by following these steps:
1. **Implement View Creation**: Create a template-heavy function that takes a GEF handle and returns a native library type. Ensure it can handle various data types and dimensionalities.
2. **Layout Check**: Implement logic to verify if the GEF resource's `Descriptor` is compatible with the library's memory model. This should check alignment, stride, and dimensionality.
3. **Fallback Logic**: Provide a path to allocate a temporary compatible buffer (ideally from a thread-local or pooled allocator) and copy/transform the data if the layout check fails.
4. **Integration with Profiler**: Ensure that every fallback copy is logged with sufficient detail (size, reason for mismatch) to assist in system optimization.
5. **Lock Management**: Ensure the adapter interacts correctly with the `Context` to maintain the integrity of the `ResourcePool` borrow rules during the native object's lifetime.

