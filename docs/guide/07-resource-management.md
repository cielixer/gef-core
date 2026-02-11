# Chapter 7: Resource Management

In previous chapters, we focused on how modules compute and how they are composed into larger structures. This chapter dives into the "lifeblood" of GEF: the data itself. We will explore how GEF manages memory, how modules access it safely, and why you—as a researcher—almost never need to worry about `new`, `delete`, or manual memory management.

## Who Owns the Data?

The most important concept in GEF resource management is the **Single Ownership Principle**. In GEF, modules do not own the data they process. They do not allocate buffers internally, and they do not decide when data should be freed.

Instead, all data is owned by the **ResourcePool**.

The ResourcePool is the central authority that manages the physical memory buffers. When a module executes, it "borrows" a view into these buffers through its bindings. This centralized ownership allows GEF to optimize memory reuse, ensure concurrency safety, and provide zero-copy integration with external libraries.

```text
    +---------------------------------------+
    |            ResourcePool               |
    |  (Owns all buffers: A, B, C, ...)     |
    +---------------------------------------+
           |                 |
    +------v------+   +------v------+
    |  Module A   |   |  Module B   |
    | (Borrows A) |   | (Borrows A) |
    +-------------+   +-------------+
```

## Managed vs. Unmanaged Pools

GEF categorizes all resources into two pools based on their lifetime requirements. As a researcher, you primarily interact with these through your binding declarations, while the system configuration determines which pool a resource lives in.

### The Managed Pool (Transient Data)
The Managed Pool is used for data that flows through your `Pipeline` or `Flow`. This is usually intermediate data—like a blurred image that is passed to a thresholding module.

- **Automatic Reference Counting**: Resources in this pool are refcounted based on the number of modules that need them as `Input`.
- **Automatic Release**: When the last module that requires a resource finishes execution (refcount reaches 0), the resource is automatically released.
- **Memory Recycling**: The pool is smart. It often reuses the memory from a released resource immediately for a new allocation in the next module, keeping the memory footprint low.

### The Unmanaged Pool (Persistent Data)
The Unmanaged Pool is for data that must survive across multiple executions of a graph or even for the entire lifetime of the system.

- **System-Controlled**: Lifetime is managed by the Engineer configuring the `System`.
- **Persistent**: Used for things like model weights, large lookup tables, or game state that accumulates over time.
- **Predictable**: No automatic deallocation happens here. The memory stays exactly where it is until the system explicitly releases it.

## Bindings: Your Views into the Pool

Bindings are not the data itself; they are your **views** into the ResourcePool. When you declare a binding in C++, you are specifying how you intend to interact with a resource.

- **Input**: A shared, read-only view. Multiple modules can have an `Input` binding to the same resource simultaneously.
- **InOut**: An exclusive, mutable view. You are granted permission to modify the data in-place.
- **Output**: A request for a new allocation. GEF will create a new resource in the pool (usually the Managed Pool) for you to fill.

### The Borrow Model
To prevent race conditions, GEF enforces a strict "Single Writer, Multiple Readers" (SWMR) borrow model at runtime:
1. You can have **multiple concurrent readers** (Input).
2. OR you can have **one exclusive writer** (InOut).
3. You can NEVER have both at the same time for the same resource.

The GEF scheduler uses your binding declarations to ensure that modules that would violate these rules never run in parallel.

## Opaque Handles and Proxy Types

You might have noticed that you never deal with raw pointers (e.g., `float*`) in GEF. Instead, the Context API returns **Proxy Types** like `InputRef<T>` or `InOutRef<T>`.

These are opaque handles that manage the borrow for you. They behave like references but ensure that you cannot accidentally hold onto a pointer after a module has finished or perform unsafe pointer arithmetic.

```cpp
GEF_MODULE(BrightnessAdjust) {
    GEF_INPUT(Image, input_image);
    GEF_INOUT(Image, target_image);
    GEF_CONFIG(float, factor);

    void execute(Context& ctx) {
        // ctx.input returns an InputRef<Image>
        auto src = ctx.input<Image>(); 
        
        // ctx.inout returns an InOutRef<Image>
        auto dst = ctx.inout<Image>();
        
        float scale = ctx.config<float>();

        // We use the proxy types as if they were the data itself
        for (int i = 0; i < src.total_pixels(); ++i) {
            dst[i] = src[i] * scale;
        }
    }
}
```

## Memory Layout: SoA vs. AoS

Efficiency often depends on how data is arranged in memory. Researchers can specify the preferred layout in the resource descriptor:

- **AoS (Array of Structures)**: The standard layout where components are interleaved (e.g., `RGB RGB RGB`). This is great for many computer vision libraries like OpenCV.
- **SoA (Structure of Arrays)**: Components are separated into contiguous blocks (e.g., `RRR... GGG... BBB...`). This is often much faster for SIMD (vectorized) operations.

GEF's ResourcePool respects these requests during allocation, ensuring your algorithm gets the most efficient data layout without you having to write custom memory management code.

## Zero-Copy Views

GEF is designed to play well with others. When you use external libraries like Eigen or OpenCV, the ResourcePool attempts to provide **Zero-Copy Views**. 

If the memory layout and alignment in the pool match what the external library expects, GEF simply passes a pointer to the existing buffer. No data is copied. If there is a mismatch, GEF will perform a copy and log a profiling warning, allowing an Engineer to optimize the descriptors later.

## Practical Advice

1. **Trust the Pool**: Don't try to manage your own long-lived buffers inside an Atomic module. Use `InOut` or ask an Engineer to configure a resource in the Unmanaged Pool.
2. **Prefer InOut for Large Data**: If you are modifying a massive tensor, using `InOut` avoids a costly allocation and copy of the entire buffer.
3. **Check Your Layouts**: If performance is lagging, check if you are using SoA for vectorizable loops.
4. **No Raw Pointers**: Never extract a raw pointer from a proxy type and store it. The proxy types are only valid for the duration of the `execute()` call.

---

### Further Reading
For a deep dive into how the ResourcePool is implemented and how it handles different hardware backends, see the [Resource Pool Architecture](../design/resource-pool.md).

**Next Chapter**: [Chapter 08: Debugging and Validation](08-debugging.md) — Learn how to catch borrow violations and binding conflicts before they hit production.
