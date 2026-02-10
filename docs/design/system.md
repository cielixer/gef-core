# System Architecture

## Overview

In the General Engine Framework (GEF), the **System** is the top-level orchestrator and the final executable product. It is the ultimate boundary of the framework; there is no higher-level concept. While modules provide the algorithmic logic and schedulers manage the execution flow, the System binds these components together into a cohesive, functional application.

The System IS the program. Whether you are building a real-time game engine, a batch video processor, or a request-response backend server, the System serves as the host that owns all resources, configures the execution environment, and manages the application lifecycle.

## The "System as the Product" Philosophy

GEF is designed with the philosophy that the framework helps you build a **System**. Unlike frameworks that provide a library of functions to be called, GEF provides a structure to be inhabited. The System is the manifestation of this structure in a specific domain.

This approach ensures that:
- **Encapsulation**: Low-level concerns like memory pooling and thread scheduling are managed at the System level, hidden from the algorithmic modules.
- **Portability**: The same algorithmic modules can be reused across different Systems (e.g., a blur module used in both a video editor and a surveillance backend).
- **Scale**: The System can be optimized for specific hardware or deployment targets without modifying the underlying algorithms.

---

## Two User Personas

GEF distinguishes between two primary roles, each interacting with the System at a different level of abstraction.

### 1. The Researcher
The **Researcher** is focused on algorithm development and experimental composition.
- **Goal**: Implement a new computer vision algorithm or a data processing flow.
- **Interactions**: Writes Atomic modules in C++, composes `Pipeline`, `Flow`, and `Batch` modules via YAML configuration.
- **System Exposure**: Minimal. The Researcher treats the System as a "black box" that executes their modules. They do not touch System-level internals like ResourcePool management or scheduler implementation.

### 2. The Engineer
The **Engineer** is focused on the System's infrastructure, performance, and deployment.
- **Goal**: Build the final product (e.g., a low-latency video streaming server).
- **Interactions**: Configures the System instance, selects and tunes the `ResourcePool` strategies, chooses the appropriate `Scheduler`, and manages persistence and external integrations.
- **System Exposure**: High. The Engineer is the primary author of the System's host code, defining how it starts, runs, and shuts down.

---

## System Responsibilities

The System acts as the central authority for several critical framework components:

1.  **ResourcePool Ownership**: The System creates and owns both **Managed** (refcounted, transient) and **Unmanaged** (persistent, manual) ResourcePools. It ensures that memory is allocated safely and recycled efficiently.
2.  **Module Discovery and Loading**: The System triggers the directory scan and plugin loading process, discovering `.so` or `.dylib` files and registering the modules within the engine.
3.  **Scheduler Selection and Configuration**: The System decides which `Scheduler` to use (Static, Dynamic, or Custom) and configures its parameters (e.g., thread pool size).
4.  **Lifecycle Management**: The System governs the transition between execution states (Setup, Configure, Run, Teardown).
5.  **YAML Integration**: The System reads the top-level configuration files to build the initial module graph and inject `Config` values.

---

## System Lifecycle

The execution of a GEF-based application follows a deterministic lifecycle managed by the System.

### 1. Setup Phase
During Setup, the System initializes its core infrastructure. This includes:
- Allocating the Unmanaged ResourcePool for persistent data (e.g., model weights).
- Initializing the plugin architecture and scanning for available modules.
- Creating the default or custom Scheduler instance.

### 2. Configure Phase
The System reads the YAML configuration to define the application's structure.
- **Graph Construction**: The System parses the `modules:` and `wiring:` fields to build the top-level DAG (usually a `Flow` or `Pipeline`).
- **Module Instantiation**: The System loads the required plugin libraries and creates instances of the requested modules.
- **Binding Validation**: The System (via the Validation Layer) ensures all `Input`, `Output`, and `InOut` bindings are correctly wired and type-compatible.
- **Config Injection**: Flat-namespace `Config` values are mapped and injected into each module instance.

### 3. Run Phase
The System hands the module graph to the Scheduler for execution.
- **Execution Model**: The System may execute the graph once (Batch processing) or repeatedly in a loop (Real-time engine).
- **Resource Management**: During execution, the System monitors the Managed ResourcePool to ensure transient data is released as soon as refcounts hit zero.

### 4. Teardown Phase
Once execution is complete, the System performs a graceful shutdown.
- Releasing all resources in both Managed and Unmanaged pools.
- Unloading plugin libraries.
- Closing external adapters (e.g., file handles, network sockets).

---

## ResourcePool Ownership and Persistence

The System is the absolute owner of all data buffers. It manages the two primary memory strategies:

- **Managed ResourcePool**: Used for transient dataflow. The System relies on the Scheduler's DAG analysis to automatically release these buffers when they are no longer needed by downstream modules.
- **Unmanaged ResourcePool**: Used for persistent data. The System manages these resources manually. This is where the System stores data that must survive across multiple "Run" cycles, such as a rolling average in a video processor or the state of a physics world in a game engine.

---

## Branch Logic (System-Level)

While standard modules (`Pipeline`, `Flow`, `Batch`) are static and structural, GEF handles runtime routing—such as `if/else` or `switch` patterns—at the System level.

**Branching** is a dynamic concept handled by the System and the Dynamic Scheduler. Instead of embedding complex control logic inside a C++ module (which makes it opaque to the framework), routing decisions are made at the System level. This allows the System to activate or deactivate entire sub-graphs based on runtime data, maintaining transparency and enabling advanced profiling of all execution paths.

*Note: Branch routing is a future capability and is currently treated as a dynamic extension of the static module graph.*

---

## System Implementation Examples

Because the System IS the product, its implementation varies based on the target domain:

### 1. Real-Time Game Engine
- **Lifecycle**: Loops the Run phase at a fixed frequency (e.g., 60Hz).
- **Resources**: Uses an Unmanaged pool for persistent world state (entities, transforms).
- **Scheduler**: Optimized for low latency and high-frequency updates.

### 2. Batch Video Processor
- **Lifecycle**: Runs the graph once per video file or once per frame sequence.
- **Resources**: Heavy use of Managed pools for frame-by-frame transient buffers (e.g., blur results).
- **Scheduler**: Optimized for throughput, likely using `Batch` modules to process frames in parallel across all CPU cores.

### 3. Backend Request-Response Server
- **Lifecycle**: Setup once, then executes the Run phase in response to external network triggers.
- **Resources**: Managed pool per-request to ensure no memory leaks between users.
- **Scheduler**: Manages multiple concurrent executions of the same graph.

---

## Relationship to YAML Configuration

The System uses YAML as its primary "instruction manual." The YAML file defines:
- Which modules to load.
- How those modules are wired together.
- Which `Config` values to inject.
- (Optionally) Which `ResourcePool` strategy to use for specific outputs.

By separating the System's structure (YAML) from its logic (C++), GEF allows the **Engineer** to reconfigure the entire product's behavior without requiring the **Researcher** to recompile a single module.
