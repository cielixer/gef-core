# Chapter 3: The Binding System

In the previous chapter, we dissected the anatomy of an Atomic Module and saw how to declare its interface using macros like `GEF_INPUT` and `GEF_OUTPUT`. However, we only scratched the surface of *how* these declarations influence the engine and *why* GEF requires you to be so explicit about your intent.

In this chapter, we will explore GEF's **Binding System**—the core contract between your algorithm and the framework's data management layer. You will learn about the four fundamental binding kinds, how they enable automatic parallelism, and how to use them to write memory-efficient code that scales.

## 1. The Four Binding Kinds

GEF categorizes all data interaction into four distinct "kinds." This model was carefully designed to be expressive enough for high-performance research while remaining safe and predictable.

| Kind | Access Intent | Proxy Type | Parallelism | Use Case |
| :--- | :--- | :--- | :--- | :--- |
| **Input** | Read-Only | `InputRef<T>` | Concurrent | Consuming data from upstream. |
| **Output** | Write (New) | `OutputRef<T>` | Independent | Producing a brand new resource. |
| **InOut** | Read + Write | `InOutRef<T>` | Exclusive | In-place mutation (memory efficient). |
| **Config** | Read-Only | `const T&` (or `T`) | Safe | Algorithm settings (kernel size, etc). |

Let's look at each in detail.

### Input: The Shared Reader
An `Input` binding represents data coming from elsewhere in the system. When you declare an `Input`, you are telling the engine: "I need to look at this data, but I promise not to change it."

Because Inputs are read-only, they are **highly parallelizable**. The GEF scheduler can run ten different modules that all read from the same `Image` at the exact same time without any risk of data corruption.

### Output: The Producer
An `Output` binding is used when your module creates something entirely new. Every time a module with an `Output` runs, the engine allocates (or pulls from a pool) a **fresh resource**. 

Outputs are the primary way data flows forward through a pipeline. If you are taking an input and creating a significantly different version of it (e.g., converting a color image to grayscale), you should use an `Output`.

### InOut: The In-Place Mutator
`InOut` is GEF's "power user" binding. It allows you to receive existing data and modify it directly in its original memory location. 

In research domains like computer vision or deep learning, creating a new 1GB tensor for every minor operation is a recipe for Out-of-Memory (OOM) errors and slow performance. `InOut` solves this by enabling **exclusive mutation**. When a module uses `InOut`, the scheduler ensures that no other module is reading or writing that specific data at the same time. It serializes access, giving your module total control over the buffer.

### Config: The Static Injector
`Config` bindings are different from the other three. While Input, Output, and InOut represent the **dataflow** (the items moving through your pipeline), `Config` represents the **environment**. 

Configs are settings like `threshold_value`, `kernel_size`, or `model_path`. They are typically loaded from a YAML file at startup and remain immutable for the lifetime of the engine. They exist in a flat namespace, meaning any module can request any config key by name.

### Summary of Declarations
To see how these look together, here is a module that utilizes all four binding kinds to perform a complex normalization:

```cpp
GEF_STAGE(Normalizer) {
    // 1. Dataflow Bindings
    GEF_INPUT(Tensor, source);      // Read-only input
    GEF_OUTPUT(Tensor, result);     // New resource produced
    GEF_INOUT(Tensor, workspace);   // Scratchpad modified in-place
    
    // 2. Engine Bindings
    GEF_CONFIG(float, scale);       // Immutable parameter
    GEF_CONFIG(std::string, mode);  // Immutable parameter

    void execute(Context& ctx) {
        // Access handles here...
    }
};
```

## 2. The Same-Name Rule

One of the most important safety features in GEF is a simple rule: **A module cannot use the same name for both an Input and an Output.**

If you try to do this, the GEF validation layer will stop you with a **BUILD ERROR**.

```cpp
// ❌ THIS WILL CAUSE A BUILD ERROR
GEF_STAGE(InvalidModule) {
    GEF_INPUT(Tensor, buffer);
    GEF_OUTPUT(Tensor, buffer); // Error: "buffer" used as both Input and Output
    
    void execute(Context& ctx) { /* ... */ }
};
```

**Why this rule exists:**
In earlier versions of GEF, researchers would often take an input named `image` and output a modified version also named `image`. This created "lineage ambiguity"—the scheduler couldn't tell if the `image` being read by a downstream module was the original or the modified one.

By banning this pattern, GEF forces you to be explicit about your intent:
1.  **If you are mutating in-place**, use `GEF_INOUT(Tensor, buffer)`.
2.  **If you are producing new data**, use `GEF_INPUT(Tensor, original)` and `GEF_OUTPUT(Tensor, processed)`.

This clarity is what allows GEF to build a deterministic and safe execution graph.

## 3. Parallelism Implications

The GEF scheduler doesn't need you to write threads or mutexes. It looks at your binding declarations and derives the parallelism automatically.

*   **Inputs and Configs** never block anything. They are "read-safe."
*   **Outputs** are independent. Producing one output doesn't block another module from producing a different output.
*   **InOut** is the "stop sign." Because it requires exclusive access, it serializes your module against any other module that wants to read or write that same data.

```ascii
      [Module A]           [Module B]
     (Input: img)         (Input: img)
          |                    |
          +----------+---------+
                     |
               (Parallel OK)
                     |
              [Module C (InOut: img)]
                     |
             (Must wait for A & B)
```

## 4. Debug-Mode Guards: "The Lazy Researcher"

Because `InOut` is so convenient, there is a temptation to use it for everything. However, overusing `InOut` kills parallelism by forcing the scheduler to serialize modules unnecessarily.

To prevent this "Lazy Researcher" anti-pattern, GEF includes a specialized check in **Debug Mode**. If you declare a binding as `InOut` but the engine detects that your module never actually wrote to that memory, it will issue a warning:

> `Warning: Module 'Threshold' declared InOut('image') but performed no writes. Consider using Input('image') for better parallelism.`

This check has zero overhead in **Release builds**, where the validation is compiled out for maximum performance.

## 5. Decision Guide: Which Binding Should I Use?

Choosing the right binding is usually straightforward. If you're unsure, follow this flow:

```ascii
Start
  |
  V
Is this a setting (e.g. kernel size, path)? --- Yes ---> [Config]
  |
  No
  |
Are you producing brand NEW data? ------------- Yes ---> [Output]
  |
  No
  |
Do you need to MODIFY existing data?
  |          |
  No        Yes
  |          |
  V          V
[Input]    Is the data huge (e.g. 4K Video)?
             |          |
            No         Yes
             |          |
             V          V
    [Output/Input]    [InOut]
    (Safer choice)   (Performance choice)
```

## 6. Running Example: Extending the Pipeline

Let's build upon the Blur pipeline from Chapter 2. We want to add a `Threshold` module. Instead of creating a new copy of the image, we'll use `InOut` to modify the blurred image in-place, and use `Config` to set the threshold level.

```cpp
#include <gef/gef.hpp>

GEF_STAGE(ImageThreshold) {
    // We modify the image in-place to save memory
    GEF_INOUT(Tensor, image);
    
    // We use Config for the algorithm parameters
    GEF_CONFIG(float, level);
    GEF_CONFIG(bool, inverse);

    void execute(Context& ctx) {
        // Retrieve our handles
        auto img = ctx.inout<Tensor>("image");
        float threshold = ctx.config<float>("level");
        bool is_inverse = ctx.config<bool>("inverse");

        // Iterate and mutate the buffer directly
        for (int i = 0; i < img.size(); ++i) {
            float val = img[i];
            bool passed = val > threshold;
            
            if (is_inverse) passed = !passed;
            
            img[i] = passed ? 1.0f : 0.0f;
        }
    }
};
```

By using `GEF_INOUT`, this module can process massive high-resolution textures without ever allocating a single extra byte of image memory.

## 7. A Historical Note

If you've followed GEF's development or looked at very old design documents, you might see terms like **State**, **View**, or **Variable**. These were early experiments in the binding model.

*   **State** was replaced by `Config` (for persistent read-only settings) and `InOut` (for persistent mutable data).
*   **View** was renamed to `Input` for clarity.
*   **Variable** was split into `Output` and `InOut` to separate "production" from "mutation."

The current 4-kind model was chosen because it provides the most explicit and unambiguous way to describe data intent.

## 8. Common Patterns and Anti-Patterns

### Pattern: The Pass-Through
Sometimes a module needs to read data and pass it along without changing it. 
**Do:** Declare it as an `Input`. Downstream modules will automatically receive the same resource handle from the engine.

### Anti-Pattern: The InOut-Everything
Declaring all your dependencies as `InOut` "just in case."
**Why it's bad:** It forces the engine to run your modules one after another, even if they could have run in parallel. It essentially turns your high-performance DAG into a slow, serial script.

### Pattern: Named Outputs for Branching
If a module can produce different types of results, use multiple `GEF_OUTPUT` declarations with descriptive names (e.g., `detected_boxes`, `confidence_scores`).

---

## What's Next?

Now that you understand how individual modules declare their data needs, it's time to see how the engine connects them together. In **Chapter 4: Composing Pipelines**, we will look at the YAML configuration system and learn how to wire outputs to inputs to build complex processing chains.

---

**Further Reading:**
*   For the technical specs of the binding model, see [Binding Architecture](../design/binding-model.md).
*   To learn more about how `InOut` interacts with memory pools, see [Resource Management](07-resource-management.md).
