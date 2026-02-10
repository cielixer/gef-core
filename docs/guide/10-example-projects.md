# Chapter 10: Example Projects

This capstone chapter synthesizes the concepts covered in the previous nine chapters by presenting three complete example projects. Each project demonstrates a different core feature of the GEF (Generic Engine Framework), from simple sequential pipelines to complex parallel graphs and data-parallel batching.

These examples are designed to show how a Researcher can compose sophisticated systems using YAML while keeping the underlying C++ Atomic modules focused and modular.

---

## Example 1: Image Processing Pipeline

The most common use case for GEF is a linear processing chain. In this example, we build a classical computer vision pipeline that takes a raw image and produces a set of edges.

### Module Graph

```text
[ Input Image ]
       |
       v
+--------------+
| GaussianBlur |
+--------------+
       |
       v
+----------------+
| ImageThreshold |
+----------------+
       |
       v
+------------+
| EdgeDetect |
+------------+
       |
       v
[ Output Image ]
```

### YAML Configuration

This pipeline uses name-matching for implicit wiring. The `blurred` output of the first module becomes the input for the next.

```yaml
name: edge_detection_pipeline
type: pipeline
inputs:
  - name: input_image
    type: Image
outputs:
  - name: output_image
    type: Image
configs:
  - sigma: 1.5
  - threshold: 128
modules:
  - name: blur
    type: GaussianBlur
    inputs:
      input: input_image
    outputs:
      output: blurred
  - name: binary
    type: ImageThreshold
    inputs:
      input: blurred
    outputs:
      output: thresholded
  - name: edges
    type: EdgeDetect
    inputs:
      input: thresholded
    outputs:
      output: output_image
```

### Key C++ Module Snippets

The `ImageThreshold` module demonstrates the use of `Input` and `Output` bindings to produce new data without modifying the source.

```cpp
GEF_STAGE(ImageThreshold) {
  GEF_INPUT(Image, input);
  GEF_OUTPUT(Image, output);
  GEF_CONFIG(int, threshold);

  void execute(Context& ctx) {
    auto in = ctx.input<Image>();
    auto out = ctx.output<Image>();
    auto thr = ctx.config<int>();

    // Abbreviated implementation
    for (int i = 0; i < in.size(); ++i) {
        out[i] = (in[i] > thr) ? 255 : 0;
    }
  }
}
```

---

## Example 2: Real-Time Video Processor

This project demonstrates how to process a video stream by treating frames as a batch. It also shows how to use the **Unmanaged ResourcePool** to maintain persistent data (like a frame counter) across multiple executions of the system.

### Module Graph

```text
+-------------+      +-------------------+
| VideoReader | ---> |   Batch Module    |
+-------------+      | (FrameProcessor)  |
                     +---------|---------+
                               |
                               v
                     +-------------------+
                     |   VideoWriter     |
                     +-------------------+
                               ^
                               |
                     +-------------------+
                     |  Unmanaged Pool   |
                     |  (Frame Counter)  |
                     +-------------------+
```

### YAML Configuration

The `type: batch` module wraps a single-frame processor. The `split` strategy partitions a video buffer into individual frames which are processed in parallel.

```yaml
name: real_time_video_system
modules:
  - name: frame_batch
    type: batch
    inputs:
      - video_chunk
    outputs:
      - processed_chunk
    split:
      strategy: batch_sequence
    inner:
      type: FrameProcessor
    mapping:
      video_chunk: input_frame
      output_frame: processed_chunk
```

### Key C++ Module Snippets

The `FrameProcessor` accesses a persistent frame counter stored in the Unmanaged pool via an `InOut` binding. This allows the module to track its position in the stream even though the Batch module executes frames in parallel.

```cpp
GEF_STAGE(FrameProcessor) {
  GEF_INPUT(Image, input_frame);
  GEF_OUTPUT(Image, output_frame);
  GEF_INOUT(int, global_frame_count);

  void execute(Context& ctx) {
    auto frame = ctx.input<Image>();
    auto& count = ctx.inout<int>(); // Persistent counter

    // Process frame...
    count++; // Atomic increment handled by framework for InOut
    
    // Output processed frame with metadata
    auto out = ctx.output<Image>();
    apply_timestamp(out, count);
  }
}
```

---

## Example 3: Scientific Computation Flow

In scientific computing, data paths often branch and merge. This example demonstrates a **Flow** (DAG) that performs parallel feature extraction and anomaly detection, followed by an aggregation step. It also showcases composition by nesting a **Pipeline** inside the Flow.

### Module Graph

```text
       [ Data Input ]
             |
      _______|_______
     |               |
     v               v
+----------+   +----------+
| Features |   | Anomaly  |
+----------+   +----------+
     |               |
     |_______ _______|
             |
             v
      +-------------+
      | Aggregator  |
      +-------------+
             |
             v
      +-------------+
      | ExportPipe  | (Nested Pipeline)
      +-------------+
             |
             v
       [ Result PDF ]
```

### YAML Configuration

The `type: flow` allows the `features` and `anomaly` modules to run in parallel. The `exporter` is a nested pipeline defined elsewhere.

```yaml
name: scientific_analysis_flow
type: flow
inputs:
  - raw_data
outputs:
  - report_pdf
modules:
  - name: features
    type: FeatureExtraction
    inputs: { data: raw_data }
  - name: anomaly
    type: AnomalyDetection
    inputs: { data: raw_data }
  - name: aggregator
    type: Aggregation
    inputs:
      feat: features.output
      anom: anomaly.output
  - name: exporter
    type: pipeline
    modules:
      - type: PDFFormatter
      - type: FileWriter
    inputs:
      input: aggregator.result
    outputs:
      output: report_pdf
```

### Key C++ Module Snippets

The `Aggregator` module uses multiple `Input` bindings to merge data from parallel branches of the Flow.

```cpp
GEF_STAGE(Aggregation) {
  GEF_INPUT(Tensor, feat);
  GEF_INPUT(Tensor, anom);
  GEF_OUTPUT(Report, result);

  void execute(Context& ctx) {
    auto f = ctx.input<Tensor>();
    auto a = ctx.input<Tensor>();
    auto& res = ctx.output<Report>();

    // Combine features and anomalies into a single report
    res.summary = combine_data(f, a);
  }
}
```

---

## Summary

These three examples demonstrate the full breadth of the GEF feature set:
*   **Modularity**: Breaking complex algorithms into small, reusable Atomic modules.
*   **Composition**: Using Pipelines for sequences, Flows for parallel DAGs, and Batching for data parallelism.
*   **Bindings**: Leveraging Input, Output, and InOut to define clear data ownership and enable automatic scheduling.
*   **Resource Management**: Mixing Managed (transient) and Unmanaged (persistent) pools to optimize memory and maintain data persistence across executions.

By defining these relationships in YAML, Researchers can iterate on system architecture without touching C++ code, while Engineers can optimize the underlying ResourcePool and Scheduler strategies to meet performance targets.

For a deeper dive into how the Scheduler resolves these graphs or how the ResourcePool manages memory at the bit-level, please refer to the [Architecture Reference](../design/overview.md).
