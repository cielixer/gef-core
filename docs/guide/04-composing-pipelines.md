# Chapter 4: Composing Pipelines

In the previous chapters, we learned how to build individual Atomic Modules and how to declare their data needs using the Binding System. While a single module like `GaussianBlur` is useful, the real power of GEF comes from **composition**—taking small, specialized modules and wiring them together to create complex processing systems.

In this chapter, we will explore the **Pipeline**, the simplest way to compose modules in GEF. We will look at how the YAML configuration allows you to define processing chains, how data flows automatically between modules using "variable list semantics," and how to handle more complex wiring scenarios.

## 1. What is a Pipeline?

A **Pipeline** is a sequential chain of modules. When a Pipeline executes, it runs its child modules one after another, in the exact order they are defined in the configuration.

Key characteristics of a Pipeline:
*   **Sequential**: Module B starts only after Module A has completely finished.
*   **Ordered**: The order of the `modules:` list in your YAML file is the order of execution.
*   **No Parallelism**: Unlike a **Flow** (which we will cover in Chapter 5), a Pipeline does not attempt to run modules in parallel, even if they have no data dependencies.

Pipelines are the bread and butter of image processing and data transformation tasks where a linear sequence of steps is the most natural way to describe the algorithm.

## 2. The YAML Configuration

In GEF, you don't write C++ code to connect modules. Instead, you use a YAML configuration file. This keeps your system architecture separate from your algorithm implementation, allowing you to reconfigure your processing chain without recompiling a single line of C++.

A basic Pipeline configuration looks like this:

```yaml
type: pipeline
name: blur_and_threshold
inputs: [image]
outputs: [processed]
configs:
  sigma: 1.5
  level: 0.5

modules:
  - GaussianBlur
  - ImageThreshold
```

### Core Fields:
*   **`type: pipeline`**: Tells the engine to use sequential execution semantics.
*   **`name`**: A unique identifier for this pipeline.
*   **`inputs:`**: A list of data items this pipeline expects to receive from the outside world.
*   **`outputs:`**: A list of data items this pipeline will produce when it finishes.
*   **`configs:`**: A set of parameters that are shared with all modules inside the pipeline.
*   **`modules:`**: An ordered list of the modules that make up the pipeline. These can be Atomic modules (referenced by their `GEF_MODULE` name) or even other Pipelines defined elsewhere.

## 3. Implicit Wiring and Variable List Semantics

One of GEF's most powerful features is **Implicit Wiring**. You'll notice in the example above that we didn't specify how the output of `GaussianBlur` connects to the input of `ImageThreshold`. 

GEF manages this through **Variable List Semantics**. As the Pipeline executes, it maintains a running list of all available data (bindings).

1.  **Start**: The list contains the items provided in the pipeline's `inputs`.
2.  **Execution**: When a module runs, the engine looks for its `GEF_INPUT` requirements in the list by name.
3.  **Production**: When a module finishes, its `GEF_OUTPUT` items are added to the list, and its `GEF_INOUT` items are updated in-place.
4.  **Propagation**: The updated list is then passed to the next module.

### Skip Connections
Because GEF maintains a running list of all outputs, you can have **Skip Connections** for free. If Module A produces an output called `metadata`, and only Module C needs it, Module B can simply ignore it. The `metadata` item remains in the list as Module B executes and is still available when Module C starts.

```ascii
[Input: img] -> [Module A] -> [Module B] -> [Module C] -> [Output: result]
                   |                           ^
                   +------- (metadata) --------+
```

## 4. Explicit Wiring: When Names Don't Match

Implicit wiring is great when names match, but sometimes you want to use a module that expects an input named `src` while your upstream module produces `blurred`. You can resolve this using **Arrow Syntax** or **Structured Wiring**.

### Arrow Syntax
The arrow syntax allows you to map an output from a specific module (or the pipeline input) to a specific input of a module.

```yaml
modules:
  - GaussianBlur:
      name: blur
  - ImageThreshold:
      wiring:
        - blur.blurred_image -> image
```

### Structured Wiring
For more complex configurations, you can use the structured wiring format:

```yaml
modules:
  - ImageThreshold:
      wiring:
        - { from: blur.blurred_image, to: image }
```

### The `inputs.` and `outputs.` Prefix
When wiring to the boundary of the Pipeline itself, you use the `inputs.` and `outputs.` prefixes. This distinguishes the Pipeline's own interface from the outputs of its child modules.

```yaml
inputs: [raw_frame]
outputs: [final_mask]

modules:
  - GaussianBlur:
      wiring:
        - inputs.raw_frame -> input_image
  - ImageThreshold:
      name: thresholder
  - EdgeDetect:
      wiring:
        - thresholder.image -> img
        - edge_map -> outputs.final_mask
```

## 5. Name Collisions and Configuration Inheritance

### Name Collisions
In a Pipeline, if two modules produce an output with the same name (e.g., both produce `temp_buffer`), GEF follows a **Last-Writer-Wins** policy. The second module's output will overwrite the first one in the variable list.

> **Note**: While GEF allows this, it will issue a **Validation Warning** during startup. It is usually better to use unique names or explicit wiring to avoid confusion.

### Config Inheritance
Configs declared at the Pipeline level are automatically inherited by all child modules. If your Pipeline defines `sigma: 2.0`, and three different modules inside the pipeline declare `GEF_CONFIG(float, sigma)`, they will all receive the value `2.0`.

If you need different modules to have different values for the same config key, you can override them in the `modules:` list:

```yaml
configs:
  sigma: 1.0  # Default for the whole pipeline

modules:
  - GaussianBlur:
      name: blur_fine
  - GaussianBlur:
      name: blur_coarse
      configs:
        sigma: 5.0  # Override for this specific instance
```

## 6. Complete Running Example

Let's build a complete 3-module pipeline for edge detection: `GaussianBlur` -> `ImageThreshold` -> `EdgeDetect`.

### C++ Module Definitions

First, our `ImageThreshold` module (building on Chapter 3):

```cpp
#include <gef/gef.hpp>

GEF_MODULE(ImageThreshold) {
    GEF_INOUT(Tensor, image);
    GEF_CONFIG(float, level);

    void execute(Context& ctx) {
        auto img = ctx.inout<Tensor>("image");
        float threshold = ctx.config<float>("level");

        for (int i = 0; i < img.size(); ++i) {
            img[i] = (img[i] > threshold) ? 1.0f : 0.0f;
        }
    }
};
```

And a simple `EdgeDetect` module:

```cpp
#include <gef/gef.hpp>

GEF_MODULE(EdgeDetect) {
    GEF_INPUT(Tensor, img);
    GEF_OUTPUT(Tensor, edges);

    void execute(Context& ctx) {
        auto input = ctx.input<Tensor>("img");
        auto output = ctx.output<Tensor>("edges");
        
        output.reshape(input.shape());

        // Simple Sobel or Laplacian implementation...
        // (Implementation omitted for brevity)
    }
};
```

### The Pipeline YAML

Now we compose them into a single file named `edge_pipeline.yaml`:

```yaml
type: pipeline
name: edge_detection_chain

# Boundary Interface
inputs:
  - raw_image
outputs:
  - final_edges

# Global Configs
configs:
  sigma: 1.2
  level: 0.4

modules:
  - GaussianBlur:
      name: blur
      wiring:
        - inputs.raw_image -> input_image
  
  - ImageThreshold:
      name: threshold
      wiring:
        - blur.blurred_image -> image
  
  - EdgeDetect:
      name: detector
      wiring:
        - threshold.image -> img
        - detector.edges -> outputs.final_edges
```

In this example, we used explicit wiring for every step to show how data moves from the pipeline input, through the internal modules (using the `module_name.binding_name` syntax), and finally to the pipeline output.

## 7. Summary and Best Practices

Pipelines are the simplest way to get started with GEF composition. By relying on sequential execution and variable list semantics, you can build complex chains with very little configuration.

### Best Practices:
1.  **Use Descriptive Names**: Even though GEF allows `module.output`, giving your modules clear names (like `noise_reduction` instead of `blur1`) makes your YAML files self-documenting.
2.  **Leverage Implicit Wiring**: Use consistent names for common data types (like `image`) across your modules to minimize the amount of explicit wiring you need to write.
3.  **Prefer Pipelines for Simple Chains**: If your logic is linear, use a Pipeline. It is easier to reason about and debug than a complex DAG.

---

In the next chapter, we'll step up the complexity and look at **Flows**. You'll learn how to express non-linear graphs (DAGs) and how GEF's scheduler uses your binding declarations to automatically run independent modules in parallel for maximum performance.

**Further Reading:**
*   [YAML Configuration Reference](../design/yaml-config.md)
*   [Module System Deep Dive](../design/module-system.md)
*   [Binding Model and Lineage](../design/binding-model.md)
