# Chapter 5: Flows and Parallelism

In the previous chapter, we explored the **Pipeline**, which executes modules in a strict sequential order. While Pipelines are excellent for simple processing chains, real-world research often involves complex tasks where multiple operations can happen at once.

In this chapter, we introduce the **Flow**. A Flow is a Directed Acyclic Graph (DAG) of modules. Unlike a Pipeline, a Flow allows independent modules to run in parallel, significantly improving performance on multi-core systems without requiring you to write complex threading code.

## 1. What is a Flow?

A **Flow** is a composite module that treats its children as nodes in a graph. The execution order is not determined by the list order in your YAML file, but by the **data dependencies** between the modules.

When a Flow starts:
1.  The scheduler analyzes the bindings of all modules.
2.  Any module whose **Inputs** are fully satisfied (either from the Flow's own inputs or from already-produced outputs) is eligible to run.
3.  Eligible modules are dispatched to the thread pool concurrently.
4.  As modules finish and produce new **Outputs**, the scheduler identifies the next set of eligible modules.

This execution model ensures that your hardware is utilized as efficiently as possible, running independent branches of your algorithm in parallel.

## 2. Flow vs. Pipeline: The Decision Guide

Choosing between a Pipeline and a Flow depends on the structure of your data processing.

| Feature | Pipeline | Flow |
| :--- | :--- | :--- |
| **Execution** | Sequential (one by one) | Parallel (where possible) |
| **Ordering** | Defined by list order | Defined by data dependencies (DAG) |
| **Wiring** | Implicit (Variable List Semantics) | Explicit (Edges define the graph) |
| **Use Case** | Linear transformations | Branching paths, parallel processing |
| **Complexity** | Simple, easy to debug | Powerful, requires clear wiring |

**Use a Pipeline when:** Your modules form a single, straight line (A -> B -> C).
**Use a Flow when:** You have branches that don't depend on each other (A -> B and A -> C), or when you want to maximize performance by running independent tasks concurrently.

## 3. YAML Configuration for Flows

The YAML syntax for a Flow is nearly identical to a Pipeline, with two key differences: the `type` field is set to `flow`, and it relies more heavily on explicit `wiring` to define the edges of the DAG.

```yaml
type: flow
name: parallel_enhancement
inputs: [image]
outputs: [enhanced_image]

modules:
  - GaussianBlur:
      name: blur
  - DenoiseFilter:
      name: denoiser
  - ImageMerge:
      name: merger
      wiring:
        - blur.blurred -> input_a
        - denoiser.clean -> input_b
        - output -> outputs.enhanced_image
```

Note that we use the `modules:` field, just like in a Pipeline. However, the order in which `blur` and `denoiser` appear doesn't matter; the scheduler sees that they both only need the Flow's input, so it runs them in parallel.

## 4. Explicit Wiring and Arrow Syntax

Because a Flow is a graph, GEF needs to know exactly how data moves between nodes. While implicit name matching still works for simple cases, Flows usually require **Explicit Wiring** to resolve ambiguities.

### Arrow Syntax
The most common way to define an edge is the arrow syntax: `source_module.binding -> target_module.binding`.

*   **`blur.blurred -> merger.input_a`**: Connects the `blurred` output of the module named `blur` to the `input_a` input of the `merger` module.
*   **`inputs.image -> blur.input_image`**: Connects the Flow's own input to a child module.
*   **`merger.output -> outputs.enhanced_image`**: Connects a child module's output to the Flow's final output.

### Structured Format
For scenarios where you need more metadata or prefer a more verbose style, you can use the structured format:

```yaml
wiring:
  - { from: blur.blurred, to: merger.input_a }
  - { from: denoiser.clean, to: merger.input_b }
```

## 5. Parallel Execution and the Scheduler

The magic of Flows happens in the **Scheduler**. Unlike other frameworks where you might manually manage threads or futures, GEF's scheduler derives parallelism automatically from your **Binding Declarations**.

Here is how the scheduler interprets your `GEF_INPUT`, `GEF_OUTPUT`, and `GEF_INOUT` macros:

1.  **Input (Read-Only)**: Multiple modules can have an `Input` binding to the same resource. The scheduler knows it is safe to run these modules concurrently.
2.  **Output (New Data)**: Every `Output` produces a unique resource. Since outputs don't conflict, modules producing different outputs can run in parallel.
3.  **InOut (In-place Mutation)**: An `InOut` binding requires **exclusive access**. If Module A is performing an `InOut` on an image, no other module can read or write that image at the same time. The scheduler will serialize these modules to ensure safety.
4.  **Config (Immutable)**: Always safe for concurrent access across all modules.

### ASCII Art Execution Diagram

Consider a flow where we preprocess an image, then run a Histogram Analysis and an Edge Detection branch in parallel, finally merging the results.

```ascii
      [ inputs.image ]
             |
             v
      +--------------+
      | Preprocessor |
      +--------------+
             |
      +------+------+
      |             |
      v             v
+-----------+ +------------+
| Histogram | | EdgeDetect |  <-- Running in Parallel
+-----------+ +------------+
      |             |
      +------+------+
             |
             v
      +--------------+
      |    Merger    |
      +--------------+
             |
             v
      [ outputs.result ]
```

## 6. Name Collisions and Conflicts

In a Pipeline, GEF handles same-named outputs using "last-writer-wins." In a **Flow**, this is strictly forbidden because modules run in parallel—there is no guaranteed "last" writer.

If two modules in a Flow attempt to produce an output with the same name, or if they both try to perform an `InOut` on the same resource without an explicit dependency, the GEF **Validation Layer** will trigger a `BindingConflict` or `InOutRace` error at startup.

To resolve these conflicts, you must use **Explicit Wiring** or rename your bindings to ensure every path in the DAG is unambiguous.

## 7. Multi-file Composition and Nesting

GEF is designed for scale. You don't have to define your entire system in one massive YAML file.

### Including Sub-modules
You can use the `include:` keyword to pull in module definitions from other files. Path resolution is always relative to the file doing the including.

```yaml
type: flow
name: main_system
modules:
  - include: modules/preprocessing.yaml
  - include: modules/analysis_flow.yaml
  - FinalAggregator
```

### Recursive Nesting
Composability is recursive in GEF. A Flow can contain a Pipeline, and a Pipeline can contain a Flow.

*   **Flow inside Pipeline**: A sequential step in your pipeline is actually a complex parallel sub-graph.
*   **Pipeline inside Flow**: One branch of your parallel graph is a simple sequential chain of atomic modules.

This allows you to organize your research into logical, reusable blocks that can be swapped or reconfigured easily.

## 8. Complete Example: Image Processing DAG

Below is a complete example of a Flow that performs parallel processing on an input image.

### C++: The Merger Module
First, a simple C++ module that combines two processed images.

```cpp
#include <gef/gef.hpp>

GEF_STAGE(ImageCompositor) {
    GEF_INPUT(Tensor, layer_a);
    GEF_INPUT(Tensor, layer_b);
    GEF_OUTPUT(Tensor, combined);

    void execute(Context& ctx) {
        auto a = ctx.input<Tensor>("layer_a");
        auto b = ctx.input<Tensor>("layer_b");
        auto out = ctx.output<Tensor>("combined");

        out.reshape(a.shape());
        
        // Simple additive blend
        for (int i = 0; i < a.size(); ++i) {
            out[i] = a[i] + b[i];
        }
    }
};
```

### YAML: The Flow Configuration
Now, the YAML file connecting the parallel branches.

```yaml
type: flow
name: image_analysis_flow
inputs: [raw_img]
outputs: [final_render]

configs:
  blur_sigma: 1.5
  edge_threshold: 0.5

modules:
  # 1. Preprocessing (runs first)
  - GaussianBlur:
      name: pre_blur
      wiring:
        - inputs.raw_img -> input_image
      configs:
        sigma: configs.blur_sigma

  # 2. Parallel Branch A: Histogram
  - HistogramCalc:
      name: hist_branch
      wiring:
        - pre_blur.blurred_image -> input_tensor

  # 3. Parallel Branch B: Edges
  - EdgeDetect:
      name: edge_branch
      wiring:
        - pre_blur.blurred_image -> img

  # 4. Merge results (runs after hist and edge finish)
  - ImageCompositor:
      name: final_merge
      wiring:
        - hist_branch.visualization -> layer_a
        - edge_branch.edges -> layer_b
        - combined -> outputs.final_render
```

In this setup, `hist_branch` and `edge_branch` will be executed at the same time by the GEF scheduler because they both depend on the output of `pre_blur` and do not depend on each other.

## 9. Conclusion

Flows represent the next level of sophistication in GEF composition. By moving from sequential pipelines to parallel DAGs, you can build high-performance systems that scale with your hardware, all while maintaining the "Researcher-First" experience of simple YAML configuration.

In the next chapter, we will look at **Batch Processing**, where we take these Modules, Pipelines, and Flows and apply them to collections of data—like processing every frame of a high-resolution video in parallel.

---

**See Also:**
*   [Scheduler Design and Strategy](../design/scheduler.md)
*   [Validation Layer: Detecting Races](../design/validation-layer.md)
*   [Binding Model Deep Dive](../design/binding-model.md)
