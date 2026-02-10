# Chapter 2: Anatomy of a Module

In the previous chapter, we saw a glimpse of how to create a simple module and run it. Now, it's time to pull back the curtain and examine the anatomy of an **Atomic Module** in detail. We will explore how macros declare your intent to the engine, how the `Context` manages data safely, and why GEF avoids traditional inheritance in favor of a clean, macro-driven plugin model.

Understanding the internal structure of a module is crucial because it is the fundamental building block you will use every day as a researcher. Whether you are implementing a cutting-edge neural network layer or a simple image filter, the patterns described here remain the same.

## 1. What is an Atomic Module?

An **Atomic Module** is the smallest unit of computation in GEF. Conceptually, you can think of it as a standalone function with a well-defined interface. It takes some inputs, potentially modifies some data in-place, and produces outputs.

Physically, an Atomic Module is a single `.cpp` file that is compiled into a shared library—a `.so` file on Linux or a `.dylib` on macOS. This "one file, one module" approach is the cornerstone of GEF's researcher-first philosophy. By isolating algorithms into standalone plugins, the framework allows you to:

*   **Isolate Complexity**: Each module focuses on a single task, making code easier to test and maintain.
*   **Fast Iteration**: When you modify a module, only that specific `.cpp` file is recompiled. The GEF engine then hot-reloads the resulting shared library.
*   **Zero Inheritance**: You never inherit from a `BaseModule` class. This avoids the "fragile base class" problem and keeps your algorithm code free from framework boilerplate.

## 2. The Macro Declarations

GEF uses a set of declarative macros to define the interface of your module. These macros aren't just for decoration; they provide the static metadata the GEF engine needs to validate your pipeline, satisfy dependencies, and schedule execution safely across multiple threads.

Every Atomic Module follows this general template:

```cpp
#include <gef/gef.hpp>

GEF_STAGE(MyAwesomeModule) {
    // 1. Declare bindings
    GEF_INPUT(Tensor, input_image);
    GEF_OUTPUT(Tensor, processed_image);
    GEF_CONFIG(float, threshold);
    GEF_INOUT(Tensor, persistent_buffer);

    // 2. Implementation
    void execute(Context& ctx) {
        // Algorithm logic goes here
    }
};
```

Let's break down each macro:

### `GEF_STAGE(ModuleName)`
This macro identifies the class as a GEF module. Under the hood, it handles the registration of your module with the engine's internal registry. Note that we don't use a string for the name here; the macro uses the identifier you provide to generate the necessary metadata.

### `GEF_INPUT(Type, name)`
Declares a **read-only** input. The engine ensures that any data provided to this binding is immutable during the module's execution. Multiple modules can read from the same input concurrently without any risk of race conditions.

### `GEF_OUTPUT(Type, name)`
Declares a **new output**. This tells the engine that your module will produce a completely new piece of data—a "resource" that did not exist before this module ran. The engine is responsible for allocating the memory for this output (managed by a `ResourcePool`).

### `GEF_INOUT(Type, name)`
Declares an **in-place mutation** binding. This is a powerful feature for performance-critical applications. It allows you to receive existing data and modify it directly. This is essential for processing large video frames or massive tensors where creating a new output copy would be too slow or memory-intensive. 

*Note: The engine serializes access to InOut bindings, ensuring only one module is mutating the data at any given time.*

### `GEF_CONFIG(Type, name)`
Declares an **immutable configuration parameter**. Unlike inputs, which represent data flowing through the graph, configs are algorithm settings (like a kernel size or a learning rate). Configs are typically loaded from your YAML file and remain constant throughout the execution of the flow.

## 3. The `execute(Context&)` Function

Every Atomic Module must implement the `execute(Context& ctx)` function. This is the heart of your module—the single entry point called by the GEF engine when all your declared inputs are ready.

The `Context` object passed to this function is your only gateway to the data. It acts as a security guard: it only allows you to access the bindings you explicitly declared in the macros above. If you try to access an input that wasn't declared with `GEF_INPUT`, the framework will catch the error at compile time or provide a clear diagnostic at runtime.

## 4. The Context API and Proxy Types

Inside the `execute` function, you interact with your data using the `Context` API. GEF is designed to be highly efficient, especially when dealing with large tensors. To achieve this while remaining easy to use, the API uses **Proxy Types**.

```cpp
void execute(Context& ctx) {
    // Returns an InputRef<Tensor>
    auto image = ctx.input<Tensor>("input_image");

    // Returns a float (value copy for simple types)
    auto t = ctx.config<float>("threshold");

    // Returns an InOutRef<Tensor>
    auto buffer = ctx.inout<Tensor>("persistent_buffer");

    // Returns an OutputRef<Tensor>
    auto result = ctx.output<Tensor>("processed_image");
}
```

### Why Proxy Types?
When you call `ctx.input<Tensor>("image")`, you might expect it to return a `const Tensor&`. However, it actually returns an `InputRef<Tensor>`.

These proxies are small, lightweight handles that point to the underlying data. They exist to solve a common problem in C++: the "accidental copy." If the API returned a reference, a researcher might accidentally write `auto image = ctx.input<Tensor>("image");` (forgetting the `&`), which would trigger a massive, slow deep-copy of the tensor.

With `InputRef<T>`, the `auto` keyword is perfectly safe. The proxy behaves like a reference but prevents deep copies unless you explicitly request one. 

*   **`InputRef<T>`**: Provides read-only (`const`) access to the data.
*   **`InOutRef<T>`**: Provides mutable access to existing data.
*   **`OutputRef<T>`**: Provides mutable access to the newly allocated resource you are producing.

## 5. Behind the Scenes: Metadata Generation

You might wonder how the GEF engine knows what bindings your module requires just by looking at a compiled shared library.

When you use the `GEF_STAGE` and binding macros, the C++ preprocessor generates a hidden `extern "C"` function in your `.so` file named `gef_get_metadata()`. This function, when called by the engine, returns a structured manifest containing:
1.  The module's name.
2.  A list of all bindings (Inputs, Outputs, InOuts, Configs).
3.  The expected types for each binding.

This is what allows GEF to perform **Static Validation**. Before the engine even starts your algorithm, it checks your YAML config against these manifests to ensure that every input is connected to a valid upstream output and that all types match. If you connect an `Image` output to a `Scalar` input, GEF will tell you exactly what went wrong before a single byte of data is processed.

## 6. A Complete Example: Gaussian Blur

Let's look at a practical example: a Gaussian Blur module. This module takes an input image, reads a `sigma` value from config, and produces a blurred output.

```cpp
#include <gef/gef.hpp>
#include <cmath>
#include <algorithm>

GEF_STAGE(GaussianBlur) {
    // 1. Interface Declaration
    GEF_INPUT(Tensor, input_image);
    GEF_OUTPUT(Tensor, blurred_image);
    GEF_CONFIG(float, sigma);

    void execute(Context& ctx) {
        // 2. Retrieve data handles
        // We use auto because the proxy types (InputRef/OutputRef) are safe.
        auto input = ctx.input<Tensor>("input_image");
        auto output = ctx.output<Tensor>("blurred_image");
        float s = ctx.config<float>("sigma");

        // 3. Algorithm setup
        // Calculate kernel radius based on sigma (3-sigma rule)
        int radius = static_cast<int>(std::ceil(s * 3.0f));
        int rows = input.rows();
        int cols = input.cols();

        // Ensure output is sized correctly (if not handled by engine)
        output.reshape({rows, cols});

        // 4. Algorithm Implementation
        // For educational purposes, this is a simple 2D convolution.
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                float val = 0.0f;
                float weight_sum = 0.0f;
                
                for (int ky = -radius; ky <= radius; ++ky) {
                    for (int kx = -radius; kx <= radius; ++kx) {
                        int py = std::clamp(y + ky, 0, rows - 1);
                        int px = std::clamp(x + kx, 0, cols - 1);
                        
                        // Gaussian weight formula
                        float dist_sq = static_cast<float>(kx*kx + ky*ky);
                        float weight = std::exp(-dist_sq / (2.0f * s * s));
                        
                        val += input(py, px) * weight;
                        weight_sum += weight;
                    }
                }
                
                // Write to the output proxy
                output(y, x) = val / weight_sum;
            }
        }
    }
};
```

This module demonstrates the perfect separation of concerns: the macros handle the "what" (data dependencies), while the `execute` function handles the "how" (the algorithm).

## 7. Module Discovery

Finally, how does the engine actually find your code? When you start a GEF-powered system, it performs a **Module Discovery** phase:

1.  **Directory Scanning**: The engine looks into the directories specified in your system configuration (typically a `build/` folder).
2.  **Plugin Loading**: It searches for shared library files (`.so` or `.dylib`). For each file, it uses the operating system's dynamic loader to open it.
3.  **Symbol Lookup**: It looks for the `gef_get_metadata` symbol we discussed earlier.
4.  **Registration**: If the symbol exists, the engine calls it, reads the manifest, and adds the module to its internal library of available tools.

Because this happens at runtime, you can add new modules to your project simply by dropping a new `.so` file into the build directory. You don't need to link them into the main GEF executable or even restart the engine in many development scenarios.

In the next chapter, we will take a deeper dive into the **Binding System** and learn how to handle more complex data relationships, including multi-buffer management and cross-module synchronization.

---

*   To learn more about how outputs are allocated, see [Resource Management](07-resource-management.md).
*   For details on the metadata format, see the [Plugin Architecture](../design/plugin-architecture.md) reference.
