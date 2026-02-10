# Batch Processing

In previous chapters, we looked at **Pipelines** for sequential execution and **Flows** for task-level parallelism. However, many research tasks involve applying the same operation to a large collection of data—such as processing every frame in a video or running a classifier on a list of images.

GEF provides the **Batch** module specifically for this "data-parallel" pattern. A Batch module is a wrapper that executes an inner module multiple times in parallel, partitioning the input data and reassembling the results automatically.

## The Batch Concept: Transparency

The most important feature of a Batch module is **transparency**. The module being wrapped (the "inner" module) does not need to know it is being run as part of a batch. It is written as if it were processing a single element.

```text
    +-------------------------------------------------------+
    | Batch Module                                          |
    |                                                       |
    |  Input Data (Collection)                              |
    |       |                                               |
    |  [ Split Strategy ] ---+                              |
    |       |                |                              |
    |       v                v                              |
    |  +--------+       +--------+            +--------+    |
    |  | Inner  |       | Inner  |            | Inner  |    |
    |  | Module |       | Module |    ...     | Module |    |
    |  +--------+       +--------+            +--------+    |
    |       |                |                              |
    |       v                v                              |
    |  [ Gather Strategy ] <-+                              |
    |       |                                               |
    |  Output Data (Collection)                             |
    +-------------------------------------------------------+
```

This design allows you to reuse any Atomic, Pipeline, or Flow module in a batched context without modifying its C++ source code.

## The Inner Module

Let's look at a simple Atomic module that performs a grayscale conversion on a single `Image`. Notice that there is no mention of loops, arrays, or threading in the C++ code.

```cpp
#include <gef.hpp>

// A simple grayscale converter designed for a single image
GEF_STAGE(GrayscaleFilter) {
    GEF_INPUT(Image, input_frame);
    GEF_OUTPUT(Image, output_frame);

    void execute(Context& ctx) {
        auto input = ctx.input<Image>();
        auto output = ctx.output<Image>();

        // Process just this one frame
        for (int y = 0; y < input.height(); ++y) {
            for (int x = 0; x < input.width(); ++x) {
                float r = input(x, y, 0);
                float g = input(x, y, 1);
                float b = input(x, y, 2);
                output(x, y) = 0.299f * r + 0.587f * g + 0.114f * b;
            }
        }
    }
}
```

When this module is wrapped in a Batch module, GEF handles the logic of calling `execute()` multiple times in parallel with different image slices.

## Split and Gather Strategies

A Batch module requires two strategies to manage data:

1.  **Split Strategy**: Defines how to partition the input collection into individual elements for the inner module.
2.  **Gather Strategy**: Defines how to reassemble the individual outputs back into a single collection.

### Common Strategies

*   **`batch_tensor_dim`**: Splits a multi-dimensional tensor along a specific dimension. This is the most common strategy for video (splitting along the time/frame dimension) or image batches.
*   **`batch_sequence`**: Splits a standard collection (like `List<T>`) into individual elements of type `T`.
*   **Custom**: You can provide your own split module if the data layout is non-standard (e.g., splitting a sparse matrix or a custom data structure).

## YAML Configuration

To use Batch processing, you define a module of `type: batch` in your YAML configuration.

```yaml
name: frame_processor_batch
type: batch
inputs:
  - video_tensor
outputs:
  - processed_video
split:
  strategy: batch_tensor_dim
  config:
    dim: 0  # Split along the first dimension (frames)
gather:
  strategy: batch_tensor_dim
  config:
    dim: 0  # Reassemble along the same dimension
inner:
  name: GrayscaleFilter  # Reference to our Atomic module
mapping:
  inputs:
    video_tensor: input_frame  # Map the slice to the inner input
  outputs:
    output_frame: processed_video # Map the inner result to the gather strategy
```

The `mapping` field is crucial: it tells GEF which input of the inner module receives the "slices" and which output should be collected.

## Complete Example: Video Processor

In this example, we wrap a more complex **Pipeline** inside a Batch module. The inner pipeline performs denoising followed by the grayscale conversion we saw earlier.

```yaml
# Define the inner processing logic as a Pipeline
name: enhance_and_convert
type: pipeline
inputs: [raw_frame]
outputs: [final_frame]
modules:
  - name: denoise
    type: DenoiseFilter
    wiring: { raw_frame: input }
  - name: gray
    type: GrayscaleFilter
    wiring: { denoise.output: input_frame }
  - name: output_boundary
    wiring: { gray.output_frame: final_frame }

---
# Wrap that pipeline in a Batch module to process the whole video
name: video_batch_processor
type: batch
inputs: [input_video]
outputs: [output_video]
split:
  strategy: batch_tensor_dim
  config: { dim: 0 }
gather:
  strategy: batch_tensor_dim
  config: { dim: 0 }
inner: enhance_and_convert
mapping:
  inputs:
    input_video: raw_frame
  outputs:
    final_frame: output_video
```

## Scheduler Distribution

When a Batch module is executed, the **Dynamic Scheduler** takes over. It determines the number of elements in the collection at runtime and creates a set of parallel tasks.

*   **Load Balancing**: The scheduler distributes these tasks across available worker threads. If one task takes longer than others (e.g., a frame with more detail), the scheduler can dynamically rebalance the remaining work.
*   **Granularity**: For very small operations, the scheduler might group multiple elements into a single task to reduce overhead.

## Safety and Race Detection

One of the most powerful features of GEF is its ability to detect data races. With Batch processing, this becomes slightly more complex because the scheduler must ensure that parallel iterations don't interfere with each other.

### InOut Overlap

If you use an **InOut** binding within a Batch module, the split strategy must guarantee that the regions provided to each iteration are disjoint.

For example, if you are performing an in-place blur on a large image by splitting it into tiles:
*   If two tiles overlap, and both iterations attempt to write to the overlapping region (via `InOut`), the scheduler will flag a **BatchOverlap** error.
*   Because data sizes (like video length or image resolution) are often only known at runtime, this check is performed at **schedule-time**, just before execution begins.

## Edge Case: Empty Collections

If a Batch module receives an empty collection (e.g., a video with zero frames or an empty list), GEF treats this as a silent success.
*   The inner module is never executed.
*   The gather strategy produces an empty output collection.
*   Depending on your system settings, a warning may be issued to the log, but the Flow will continue to execute.

## When to use Batch

Use a **Batch** module when:
*   The operation is identical for every element in the collection.
*   The elements are independent (or can be processed in independent blocks).
*   You want to leverage all available CPU/GPU cores without writing threading code.

If the logic needs to change based on the element index, or if there are dependencies between iterations (like temporal smoothing in video), you should use a standard **Flow** or a custom Atomic module instead.

## Next Steps

Now that you can process large datasets in parallel, you may start worrying about the memory footprint of these collections. In **Chapter 07: Resource Management**, we will explore how GEF manages the lifetime of these large tensors and how to use ResourcePools to keep your memory usage under control.

---

**Cross-references:**
*   [Chapter 03: Bindings](03-bindings.md) - Learn more about Input, Output, and InOut.
*   [Chapter 05: Flows and Parallelism](05-flows-and-parallelism.md) - Task-level parallelism basics.
*   [Architecture: Scheduler](../design/scheduler.md) - Deep dive into how the scheduler works.
