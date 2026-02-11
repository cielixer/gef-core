# Chapter 9: Working with External Libraries

GEF is designed as a "glue" framework. We recognize that as a researcher, you already have favorite libraries for linear algebra, computer vision, and signal processing. GEF doesn't try to replace Eigen, OpenCV, or ArrayFire; instead, it provides a seamless bridge to them.

This chapter explores how GEF integrates with external libraries using the **Open Adapter Pattern**, ensuring you can use native library types while the framework handles the heavy lifting of memory management and concurrency.

## The Adapter Pattern

At the heart of GEF's integration strategy is the **Adapter**. Think of an adapter as a translator. On one side, it speaks GEF: it understands `ResourcePool` handles, `Tensor` descriptors, and the framework's borrow rules. On the other side, it speaks the native language of an external library, such as `Eigen::Matrix` or `cv::Mat`.

When you use an adapter within a module, it bridges these two worlds. It allows you to wrap GEF-managed memory in a native object that the external library can operate on directly.

```text
+-------------------+       +-------------------+       +-------------------+
|   GEF Resource    |       |    GEF Adapter    |       |  External Library |
| (ResourcePool)    | <---> | (Type Translation)| <---> |  (e.g., Eigen)    |
+-------------------+       +-------------------+       +-------------------+
          ^                           |                           |
          |                           v                           |
          +------------------- Direct Memory View ----------------+
```

From your perspective as a researcher, this means you can write modules that look and feel like native code for your favorite library, without worrying about how the data got there or where it goes next.

## Zero-Copy Performance

The primary goal of the adapter pattern is **Zero-Copy Performance**. In high-performance computing, copying data is the enemy. GEF strives to ensure that when you pass data to an external library, that library is working on the *exact same bytes* that GEF is managing.

### Zero-Copy Views
When the memory layout of a GEF resource matches what the external library expects, the adapter creates a direct **view** (or "map") over the existing buffer. No data is moved. No new memory is allocated. This happens when:

1.  **Alignment**: The memory is aligned to the library's requirements (e.g., SIMD alignment).
2.  **Stride**: The "jump" between rows or elements in memory matches the library's internal model.
3.  **Data Type**: The primitive types (float, int, etc.) are identical.

In these cases, your external library operations run at the absolute maximum speed possible on the hardware.

### Copy Fallback
Sometimes, a perfect match is impossible. For instance, if you are working on a non-contiguous sub-region of a larger image, or if a library requires a specific 64-byte alignment that the input data lacks.

In these scenarios, GEF adapters perform a **Copy Fallback**. The adapter will:
1.  Allocate a temporary compatible buffer.
2.  Copy your data into that buffer.
3.  Let the library process the copy.
4.  Copy the results back (if it's an `InOut` or `Output` binding).

While this is slower than a zero-copy view, it ensures your code *always works*. 

> **Optimization Tip**: GEF logs every fallback copy in its profiling output. If you see "Alignment mismatch" or "Stride copy" in your logs for a performance-critical module, it's a signal to an Engineer to adjust the upstream data layout to enable zero-copy.

## Primary Example: Eigen Integration

Eigen is the gold-standard linear algebra library for C++, and GEF provides first-class support for it. The GEF Eigen adapter uses `Eigen::Map` to create views over `ResourcePool` memory.

### Simple Matrix Operation
In this example, we create a module that takes an input tensor and performs a simple Eigen-based transformation.

```cpp
#include <gef/module.hpp>
#include <gef/adapters/eigen.hpp>

// A module that scales a matrix using Eigen
GEF_MODULE(MatrixScaler) {
    GEF_INPUT(Tensor<float, 2>, input_matrix);
    GEF_OUTPUT(Tensor<float, 2>, output_matrix);
    GEF_CONFIG(float, scale_factor);

    void execute(Context& ctx) {
        // Create Eigen views over GEF resources
        // gef::eigen::map handles the zero-copy logic automatically
        auto mat_in = gef::eigen::map(ctx.input(input_matrix));
        auto mat_out = gef::eigen::map(ctx.output(output_matrix));
        
        float factor = ctx.config<float>("scale_factor");

        // Use native Eigen syntax!
        mat_out = mat_in * factor;
    }
}
```

### Complex Operation: Image Processing with Eigen
Because GEF `Image` types are often compatible with Eigen's memory model, you can use Eigen to perform advanced operations on pixel data, such as applying a transformation matrix for color correction.

```cpp
#include <gef/module.hpp>
#include <gef/adapters/eigen.hpp>

// Applying a 3x3 color transformation matrix to an image
GEF_MODULE(ColorTransformer) {
    GEF_INPUT(Image, input_image);       // 3-channel RGB image
    GEF_OUTPUT(Image, output_image);
    GEF_CONFIG(Tensor<float, 2>, transform_matrix); // 3x3 matrix

    void execute(Context& ctx) {
        auto img_in = ctx.input(input_image);
        auto img_out = ctx.output(output_image);
        
        // Map the 3x3 config matrix to Eigen
        auto T = gef::eigen::map(ctx.config<Tensor<float, 2>>("transform_matrix"));

        // Treat the image pixels as a large matrix (Channels x Pixels)
        // Note: This assumes interleaved RGB data
        auto pixels_in = gef::eigen::map_pixels(img_in);
        auto pixels_out = gef::eigen::map_pixels(img_out);

        // Perform the transformation across all pixels at once
        pixels_out = T * pixels_in;
        
        // Result is written directly into the ResourcePool's output buffer
    }
}
```

## The Open Adapter Pattern

GEF is built on the principle of extensibility. The "Open" in Open Adapter Pattern means that the framework doesn't have a closed list of supported libraries. While we provide the Eigen adapter out-of-the-box, the architecture allows anyone to write an adapter for any library.

### Future Adapters
The GEF roadmap includes several official adapters currently in development:
- **OpenCV**: Map GEF `Image` resources directly to `cv::Mat`.
- **ArrayFire**: Seamlessly move GEF tensors into `af::array` for GPGPU acceleration.

### Writing Your Own Adapter
If you use a specialized or internal library, you can follow these guidelines to create your own adapter:

1.  **Check Layouts**: Compare your library's expected stride and alignment with GEF's `Descriptor`.
2.  **Map if Possible**: Use the library's "external data" or "map" type (like `Eigen::Map` or `cv::Mat` constructors that take a pointer) to point to GEF memory.
3.  **Log Fallbacks**: If you must copy, use the GEF logging API so the framework can track the performance impact.
4.  **Respect Borrows**: Ensure the native object's lifetime is contained within the module's `execute()` function to maintain GEF's safety guarantees.

## Summary

External libraries are first-class citizens in the GEF ecosystem. By using the adapter pattern, you get the best of both worlds: the safety and management of a modern framework, and the raw power of established high-performance libraries.

In the next chapter, we will put everything we've learned together by looking at **Example Projects**—complete systems built with GEF that demonstrate complex pipelines and real-world performance.

---
*For a deeper dive into the technical implementation of adapters and memory alignment rules, see the [External Adapters Architecture](../../design/external-adapters.md).*
