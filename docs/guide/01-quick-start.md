# Chapter 1: Quick Start

Welcome to your first hands-on experience with GEF. In this chapter, we will set up your development environment, create a minimal "Hello World" module, and run your first image processing pipeline. By the end of this section, you will understand the basic build-and-run cycle that makes GEF such a powerful tool for rapid algorithm development.

## 1. Prerequisites

Before we begin, ensure your system has the following tools installed:

*   **C++23 Compiler**: GEF leverages the latest C++ features. You'll need a modern compiler like GCC 13+, Clang 16+, or MSVC 19.36+.
*   **pixi**: The primary package and environment manager for GEF. It handles all dependencies, including the compiler and build tools, ensuring a consistent environment across different machines.
*   **CMake**: The industry-standard build system generator used by GEF to manage the compilation of modules.

If you don't have `pixi` yet, you can install it with a single command from [pixi.sh](https://pixi.sh).

## 2. Setting Up the Project

GEF uses `pixi` to manage the development environment. This means you don't have to manually install complex C++ libraries or worry about version mismatches.

To initialize a new GEF project, create a directory and run:

```bash
mkdir my-gef-project
cd my-gef-project
pixi init
```

Next, we add the GEF framework as a dependency (this will be available once GEF is officially released):

```bash
pixi add gef
```

This command automatically pulls in the necessary headers, libraries, and the GEF runtime.

## 3. Project Structure Layout

A typical GEF project is organized to separate algorithm logic from system configuration. Here is the layout we'll be using:

```text
my-gef-project/
├── modules/            # Your C++ source files (.cpp)
├── configs/            # Pipeline and Flow definitions (.yaml)
├── build/              # Compiled shared libraries (.so / .dylib)
└── pixi.toml           # Project environment configuration
```

*   **`modules/`**: This is where your **Atomic Modules** live. Each `.cpp` file here corresponds to a single computation unit.
*   **`configs/`**: This directory contains the YAML files that define how your modules are composed into **Pipelines** or **Flows**.
*   **`build/`**: When you build a module, GEF compiles it into a shared library (plugin) located here. The system dynamically loads these at runtime.

## 4. The "Hello World" of GEF

Let's create our first module. In GEF, the simplest possible module is an **Atomic Module** that takes an input and produces an output. In our image processing domain, this could be a "Grayscale" filter that converts a color image to black and white.

Create a file named `modules/grayscale.cpp`:

```cpp
#include <gef/gef.hpp>

// 1. Declare the module using the GEF_MODULE macro
GEF_MODULE(GrayscaleModule) {
    // 2. Define the interface bindings
    GEF_INPUT(Tensor, input_image);    // Read-only input image
    GEF_OUTPUT(Tensor, output_image);  // New output image we will produce

    // 3. Implement the execution logic
    void execute(Context& ctx) {
        // Retrieve the input tensor (read-only)
        auto img = ctx.input<Tensor>("input_image");
        
        // Prepare the output tensor
        auto result = ctx.output<Tensor>("output_image");
        
        // Algorithm logic: Convert RGB to Grayscale
        // (Assuming standard HWC tensor layout)
        for (int i = 0; i < img.rows(); ++i) {
            for (int j = 0; j < img.cols(); ++j) {
                float r = img(i, j, 0);
                float g = img(i, j, 1);
                float b = img(i, j, 2);
                result(i, j) = 0.299f * r + 0.587f * g + 0.114f * b;
            }
        }
    }
};
```

### Breaking it Down
*   **`GEF_MODULE`**: This macro registers your class as a GEF module.
*   **`GEF_INPUT` / `GEF_OUTPUT`**: These declare your data dependencies. GEF uses explicit **Input** (read-only) and **Output** (new data) bindings to ensure data safety and clarity.
*   **`execute(Context& ctx)`**: This is the heart of the module. The `Context` provides safe access to the data you declared in your bindings.

## 5. Configuring the Pipeline

Now that we have a module, we need to tell GEF how to run it. We do this using a YAML configuration file.

Create `configs/hello_world.yaml`:

```yaml
module:
  name: hello_pipeline
  type: pipeline
  inputs:
    - name: image
      type: Tensor
  outputs:
    - name: result
      type: Tensor
  modules:
    - name: grayscale
      type: GrayscaleModule
```

In this config:
*   We define a **Pipeline** named `hello_pipeline`.
*   We specify that the pipeline itself takes one input (`image`) and produces one output (`result`).
*   The `modules:` list contains our `GrayscaleModule`. Because the names match (implicitly or explicitly), GEF knows to route the pipeline's input to the module's input.

## 6. Build and Run Cycle

The magic of GEF lies in its "hot-reload" process. When you modify an algorithm, you only rebuild the specific module that changed.

### The Cycle in Action

```text
  +-------------------+
  | Modify .cpp file  | <-----------+
  +---------+---------+             |
            |                       |
            v                       |
  +-------------------+             |
  | Run `pixi run build`| (Rebuilds only changed .so)
  +---------+---------+             |
            |                       |
            v                       |
  +-------------------+             |
  | Run `pixi run gef`| (Hot-reloads and executes)
  +-------------------+             |
            |                       |
            +-----------------------+
```

To build your module:

```bash
pixi run build modules/grayscale.cpp
```

To run the pipeline:

```bash
pixi run gef configs/hello_world.yaml --input data/test_image.png
```

## 7. What Just Happened?

When you ran the commands above, GEF performed several steps behind the scenes:

1.  **Plugin Compilation**: The `pixi run build` command invoked CMake to compile your `grayscale.cpp` into a shared library (`grayscale.so` or `grayscale.dylib`).
2.  **Configuration Parsing**: The GEF runtime read your `hello_world.yaml` and validated the module graph.
3.  **Module Discovery**: GEF scanned the build directory, found the `GrayscaleModule` plugin, and loaded it dynamically.
4.  **Static Scheduling**: The framework analyzed the bindings. Since there was only one module, the schedule was simple: provide the input image, call `execute()`, and capture the output.
5.  **Execution**: Your C++ code ran, processing the pixels and producing the grayscale result.

## 8. Summary

Congratulations! You've just completed the "Hello World" of GEF. You've seen how easy it is to:
*   Set up a project with **pixi**.
*   Write an **Atomic Module** using macros and the **Context API**.
*   Compose modules into a **Pipeline** via YAML.
*   Experience the fast build-and-run cycle.

In the next chapter, we'll dive deeper into writing more complex modules and explore the different ways you can manipulate data using GEF's powerful binding system.

---

For more details on the plugin system, see [Plugin Architecture](../design/plugin-architecture.md).
