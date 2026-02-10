# What is GEF?

Welcome to the General Engine Framework (GEF) tutorial. Whether you are a researcher developing new algorithms for image analysis or an engineer building the next generation of real-time video processing systems, GEF provides the foundation for modular, high-performance computation.

GEF is a C++23 framework designed to simplify the development of complex, data-driven applications. It focuses on modularity, clear data ownership, and high-performance execution. By separating the logic of individual computation units from the way they are composed and executed, GEF allows you to build scalable systems that are both easy to reason about and incredibly fast.

> **Design Status Disclaimer**
> GEF is currently in the design phase. Code examples in this tutorial show the intended API surface. The framework is not yet implemented.

## The Problem GEF Solves

In modern research and engineering, we often find ourselves caught between two extremes. On one hand, we have monolithic applications that are difficult to modify and slow to build. These "spaghetti-code" engines often bury core algorithmic logic under layers of infrastructure, making it nearly impossible for a researcher to experiment with a single part of the pipeline without triggering a cascade of recompilation and side effects.

On the other hand, we have flexible scripting environments and high-level frameworks that provide great developer experience (DX) but lack the raw performance, deterministic memory management, and safety required for production systems.

GEF bridges this gap. It provides a "researcher-first" developer experience where you can modify a single algorithm, rebuild just that module, and see the results immediately. It achieves this by using a plugin-based architecture and a declarative data-flow model, all while maintaining the performance of C++23 and the safety of a strongly-typed framework.

## Architecture at a Glance

GEF organizes computation into a hierarchy of modules managed by a central System. Below is a high-level overview of how these components fit together:

```
System (the program)
  |
  +-- ResourcePool (Managed / Unmanaged)
  +-- Scheduler (Static + Dynamic)
  +-- Module Graph
        +-- Atomic Modules (.cpp -> .so)
        +-- Pipeline (sequential)
        +-- Flow (DAG, parallel)
        +-- Batch (data-parallel)
```

At the core of GEF is the **System**, which owns the resources and orchestrates execution. Data flows through a **Module Graph**, which can range from simple sequential **Pipelines** to complex parallel **Flows** and data-parallel **Batches**. Every piece of data is stored in a **ResourcePool**, ensuring clear ownership and lifetime management.

## Philosophy

GEF is built on a set of core principles that guide every design decision. Understanding these will help you make the most of the framework.

### Explicit Control over Implicit Magic
We believe that code should be easy to reason about. In GEF, data dependencies are never hidden behind hidden global data or implicit side effects. Every module explicitly declares its interface:
- **Input**: Data that the module reads but cannot modify.
- **Output**: New data that the module produces.
- **InOut**: Data that the module modifies in-place for maximum efficiency.
- **Config**: Immutable settings injected by the framework.

This explicitness allows both the developer and the framework to understand exactly how data moves through the system, making debugging and optimization much simpler.

### Safe Concurrency without Sacrificing Performance
Modern hardware is parallel by nature, but writing thread-safe code is notoriously difficult. GEF's scheduler uses your module declarations to automatically extract parallelism. If two modules only read from the same data (via **Input**), they can run concurrently. If one modifies data (**InOut**) that another reads, the framework ensures they run sequentially. You get the benefits of multi-core execution without the headache of manual thread management or mutexes.

### Single Ownership of All Resources
Memory management is a common source of bugs in high-performance C++. GEF centralizes memory management in **ResourcePools**. Resources have a clear lifecycle:
- **Managed ResourcePool**: Uses automatic reference-counting for transient data flowing within an execution.
- **Unmanaged ResourcePool**: Gives the System full control over persistent data that must survive across multiple runs.
This model ensures that you never have to worry about manual memory management or unexpected memory leaks.

### Researcher-Friendly DX
Iteration speed is everything in algorithm development. GEF uses a plugin-based architecture where each **Atomic Module** is defined in its own `.cpp` file and compiled into a shared library (`.so` or `.dylib`). When you change an algorithm, you only rebuild that one module. The system hot-reloads the change, allowing you to iterate on complex pipelines in seconds. No more waiting for the entire project to relink.

### Data-Oriented Design (DOD)
GEF is designed for high-throughput data processing. By embracing Data-Oriented Design principles, we prioritize memory layout and efficient data access. The **InOut** binding specifically allows for in-place mutation of large buffers—like high-resolution images or video frames—minimizing unnecessary allocations and cache misses. We treat data as a first-class citizen, ensuring that the framework stays out of the way of the hardware.

## Two Personas

GEF is designed to support a collaborative flow between two primary roles:

### The Researcher
The Researcher is the primary author of algorithms. They spend most of their time writing **Atomic Modules** in C++ and composing them into larger structures using YAML configuration files. They focus on mathematical correctness, iteration speed, and ease of experimentation. GEF empowers them to run their code immediately without worrying about the underlying "plumbing" of the system.

### The Engineer
The Engineer is responsible for building the final **System** (the product). They configure the ResourcePool strategies, choose the right Scheduler, and integrate GEF into a production environment—whether it's a game engine, a real-time video processor, or a backend server. They focus on deployment, stability, and overall system performance.

| Persona | Responsibility | Primary Tools |
| :--- | :--- | :--- |
| **Researcher** | Algorithm development, module authorship | C++ Atomic Modules, YAML configs |
| **Engineer** | System infrastructure, resource management | System configuration, ResourcePools |

## The Running Example: Image Processing

Throughout this book, we will use **Image Processing** as our running example domain. Images are a perfect way to demonstrate GEF's strengths:
- They are **visually intuitive**, making it easy to see if an algorithm is working.
- They are **computationally intensive**, showcasing GEF's performance.
- They often involve **parallel tasks** (like processing different color channels or video frames).
- They require **efficient memory management** for high-resolution data.

We will build a pipeline that reads an image, applies filters like blur and edge detection, and eventually processes video sequences in parallel.

## Why C++23?

GEF is built on the latest C++ standards to provide the best possible developer experience. C++23 offers powerful new features like `std::expected` for better error handling, improved `ranges` for data manipulation, and more expressive template metaprogramming. By targeting C++23, we ensure that the framework remains modern, efficient, and capable of utilizing the latest advancements in compiler technology.

## Building the Future of Engines

GEF is more than just a library; it is a way of thinking about computation. By formalizing the way data moves and modules interact, we create a platform where innovation can happen at the speed of thought. As the ecosystem grows, we envision a future where researchers can share modules as easily as they share ideas, and where complex engines can be assembled as simply as building with blocks.

## What You'll Learn

This book is organized as a sequential journey through the GEF framework. Each chapter introduces a new concept and builds on the last.

1.  **Chapter 1: Quick Start** — Setting up your environment with `pixi` and running your first "Hello World" pipeline. We'll cover the installation of the GEF toolchain and how to execute a pre-built example to verify your setup.
2.  **Chapter 2: Your First Module** — Writing a simple image blur module in C++ using the `GEF_STAGE` macro. You'll learn the basic structure of an Atomic Module and how to implement the `execute` function.
3.  **Chapter 3: Understanding Bindings** — A deep dive into the four binding kinds: Input, Output, InOut, and Config. This is the heart of GEF's data-flow model, where you'll learn how to declare your data dependencies safely and explicitly.
4.  **Chapter 4: Composing Pipelines** — Learning how to link modules together in a sequential, linear chain via YAML. We'll explore how simple name-matching allows you to build complex logic from small, reusable units.
5.  **Chapter 5: Flows and Parallelism** — Building complex Directed Acyclic Graphs (DAGs) and letting the scheduler run them in parallel. You'll learn how to express non-linear data flows and watch the framework optimize execution across multiple CPU cores.
6.  **Chapter 6: Batch Processing** — Using Batch modules to process collections of data (like video frames) concurrently. This chapter shows how to scale your algorithms from single images to entire video streams with minimal extra code.
7.  **Chapter 7: Resource Management** — Understanding the difference between Managed and Unmanaged ResourcePools. We'll discuss when to let the framework handle lifetimes and when you need to take manual control for persistent data.
8.  **Chapter 8: Debugging and Validation** — Leveraging GEF's validation layer to catch binding conflicts and race conditions early. We'll show how GEF's static and runtime checks can save you hours of debugging.
9.  **Chapter 9: External Libraries** — Integrating powerful C++ libraries like Eigen and OpenCV into your GEF modules. We'll explore the adapter pattern and how to bring your favorite tools into the GEF ecosystem.
10. **Chapter 10: Example Projects** — A look at end-to-end applications, from real-time video processors to game engine systems. We'll wrap up by looking at how all these concepts come together in real-world software.

## How to Read This Book

This tutorial is designed to be read sequentially. Each chapter provides hands-on exercises that build upon the code you wrote in previous sections. We strongly recommend writing the code yourself rather than just reading the text.

We have carefully crafted this tutorial to be accessible to anyone with a basic understanding of C++ and a desire to build better systems. You don't need to be an expert in concurrency or memory management—GEF handles the hard parts so you can focus on your algorithms.

While this book focuses on the "how-to" of using GEF from a researcher's perspective, we often pause to explain the design rationale behind our decisions. For those who want to dive into the implementation details or build their own GEF-based systems, we provide links to the [Architecture Reference](../design/overview.md) at the end of each chapter.

Let's get started by setting up your environment in the next chapter!

---

For design rationale and implementation details, see the [Architecture Reference](../design/overview.md).
