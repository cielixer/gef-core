# Resource Pool Architecture

## Overview

The `ResourcePool` is the central memory management authority in GEF. It operates on the **Single Ownership Principle**, meaning the pool (or the `System` that owns the pool) is the absolute owner of all data buffers used within the engine. Modules never own data; they only "borrow" it through well-defined bindings.

By centralizing data ownership, the `ResourcePool` enables:
- **Lifetime Management**: Automatic or manual cleanup of resources based on usage.
- **Concurrency Safety**: Enforcement of borrow rules at the handle level.
- **Memory Reuse**: Smart allocation strategies that recycle buffers for transient data.
- **Backend Abstraction**: Decoupling the module's logical data access from physical memory (CPU, GPU, etc.).

## Two Pool Types

GEF distinguishes between data that flows through a graph and data that persists across executions. This is handled by two distinct pool types.

### 1. Managed ResourcePool
The Managed Pool is designed for **transient data**—the intermediate results that flow between modules in a `Pipeline` or `Flow`.

- **Automatic Lifetime**: Resources in this pool are reference-counted.
- **Refcount Logic**: A resource's reference count is determined by its active consumers in the scheduled DAG. 
- **Release Strategy**: When a resource's refcount reaches zero (meaning all downstream modules that needed it have finished execution), the resource is automatically marked for release or immediate reuse.
- **Optimization**: This pool frequently reuses memory blocks for different resources of the same size, significantly reducing allocation overhead in high-throughput pipelines.

### 2. Unmanaged ResourcePool
The Unmanaged Pool is designed for **persistent state**—data that must survive across multiple executions of a DAG or remain constant for the lifetime of the `System`.

- **Manual Lifetime**: There is no automatic reference counting. The `System` (configured by an Engineer) explicitly allocates and releases these resources.
- **Persistence**: Used for model weights, large lookup tables, persistent caches, and state that needs to be accumulated over time.
- **Predictability**: Since no automatic deallocation occurs, the memory footprint of the unmanaged pool is highly stable and predictable.

---

## Resource Lifecycle FSM

Every resource in the pool moves through a Finite State Machine (FSM) that governs its valid operations.

```text
  +-------------+       allocate()       +-------------+
  |             |  --------------------> |             |
  | Unallocated |                        |  Allocated  |
  |             |  <-------------------- |             |
  +-------------+        release()       +-------------+
         ^                                      |
         |                                      | bind()
         |                                      v
  +-------------+       unbind()         +-------------+
  |             |  <-------------------- |             |
  |  Released   |                        |    Bound    |
  |             |  <-------------------- |             |
  +-------------+       refcount=0       +-------------+
         ^                                      |
         |                                      | borrow()
         |                                      v
         |        release_borrow()       +-------------+
         +------------------------------ |             |
                                         |  Borrowed   |
                                         |             |
                                         +-------------+
```

### State Transitions:
1. **Unallocated**: The initial state where no memory or handle exists.
2. **Allocated**: Memory is reserved, and a handle is generated.
3. **Bound**: The resource is assigned to a specific named binding (Input/Output/InOut) in the execution context.
4. **Borrowed**: A module has acquired an active lock (Read or Write) on the resource and is currently processing its data.
5. **Released**: The resource is no longer needed. In managed pools, this happens when refcount hits zero. In unmanaged pools, this is an explicit system call.

---

## Opaque Handles

To maintain strict ownership and prevent pointer-related bugs (like Use-After-Free), the `ResourcePool` never exposes raw pointers to the rest of the framework. Instead, resources are accessed via **Opaque Handles**.

- **Handle Content**: A handle is a lightweight, typed identifier containing an index into the pool's registry and a generation ID (to prevent ABA problems).
- **Type Safety**: Handles are templated (e.g., `Handle<Tensor>`, `Handle<Image>`) to ensure modules receive the correct data type.
- **Security**: Because handles are not pointers, modules cannot perform pointer arithmetic or accidentally access memory outside their borrowed bounds.

## Resource Descriptor

Every resource in the pool is defined by a `Descriptor`. This metadata allows the pool to allocate the correct amount of memory and enables the validation layer to check compatibility.

A `Descriptor` includes:
- **dtype**: The underlying data type (e.g., `float32`, `uint8`, `int64`).
- **shape**: The dimensions of the data (e.g., `[1920, 1080, 3]` for an image).
- **layout**: The memory organization strategy.
  - **SoA** (Structure of Arrays): All values for a component are contiguous (e.g., all Red pixels, then all Green).
  - **AoS** (Array of Structures): Components for a single element are contiguous (e.g., R, G, B, R, G, B).
- **stride**: The byte offset between adjacent elements in each dimension.
- **alignment**: The memory alignment requirements (important for SIMD and GPU performance).

---

## Borrow Rules

The `ResourcePool` enforces strict **Borrow Rules** to prevent race conditions. These rules are analogous to Rust's borrow checker but are applied at runtime by the scheduler and pool.

### The SWMR Rule (Single Writer Multiple Readers)
For any given resource handle at any point in time:
- **Multiple Reads**: Any number of modules may borrow the resource for **Input** (Read-only) concurrently.
- **Single Exclusive Write**: Only **one** module may borrow the resource for **InOut** (Write/Mutate) at a time.
- **Exclusion**: An exclusive write borrow cannot be granted if there are any active read borrows, and vice versa.

The scheduler uses these rules to determine which modules can run in parallel. If Module A has an `InOut` binding to `BufferX`, and Module B has an `Input` binding to the same `BufferX`, the scheduler ensures they never run at the same time.

---

## Memory Layout Options

Efficiency is a core goal of GEF. Different algorithms perform better with different memory layouts. The `ResourcePool` supports specifying the layout in the descriptor.

- **SoA Support**: Ideal for vectorization (SIMD) where operations are performed on many elements of the same channel simultaneously.
- **AoS Support**: Ideal for many standard computer vision libraries (like OpenCV) and hardware texture units that expect interleaved channels.

The `ResourcePool` allocator respects these layout requests, ensuring that the allocated memory stride and pitch are compatible with the requested organization.

## Zero-Copy Views

One of GEF's primary strengths is its ability to integrate with external libraries (Eigen, OpenCV, ArrayFire) with zero overhead.

### Mechanism
When a module requests a view into a borrowed resource, the `ResourcePool` checks if the resource's descriptor (stride, alignment, layout) matches the requirements of the external library.
- **Success**: The pool provides a direct view (e.g., `cv::Mat` wrapping the raw buffer) without copying any data.
- **Failure**: If the requirements do not match (e.g., the external library requires a specific alignment that the resource lacks), the pool must perform an **explicit copy**.

### Profiling
Whenever an explicit copy is forced due to a layout or alignment mismatch, GEF records a **Profiling Log**. This allows Engineers to identify performance bottlenecks and adjust descriptors in the YAML configuration to enable zero-copy paths.

---

## Relationship to Bindings

Bindings (defined in `binding-model.md`) are the researcher-facing frontend of the data system, while the `ResourcePool` is the engineer-facing backend.

- **Input Binding**: Represents a **Shared Read Borrow** from the pool.
- **Output Binding**: Requests the **Allocation** of a new resource (usually in the Managed Pool).
- **InOut Binding**: Represents an **Exclusive Write Borrow** from the pool.

The `ResourcePool` serves as the fulfillment layer that turns these logical declarations into physical memory access.

---

## Module Pool Selection

While the `System` usually decides which pool to use based on global configuration, GEF allows for **Advanced Manual Override**.

In the YAML configuration for a module, an Engineer can specify that a particular output should be allocated in a specific pool:

```yaml
module:
  name: MyPersistentModule
  outputs:
    - name: cumulative_average
      pool: unmanaged  # Explicit override
```

This level of control is essential for complex systems where most data is transient, but specific modules need to maintain state across runs without the Researcher needing to change their C++ code.

---

## Design Evolution

The current Two-Pool model is the result of significant architectural simplification.

### The Old 3-Scope Model
Previously, GEF utilized a complex "Scope" hierarchy:
- **Module Scope**: Data existed only for the duration of a single module.
- **Consistent Scope**: Data persisted for the duration of a `Pipeline` or `Flow`.
- **Global Scope**: Data persisted for the life of the `System`.

Moving data between these scopes required an explicit and often confusing `promote()` function.

### Why the Model Changed
The 3-scope model was found to be overly complicated for both researchers and engineers. It didn't map well to the actual hardware requirements and created significant cognitive load.
1. **Terminology Confusion**: The distinction between "Module" and "Consistent" was often blurred in practice.
2. **Promotion Overhead**: The `promote()` mechanism introduced hidden complexities in the scheduler and made the DAG harder to analyze statically.
3. **Managed/Unmanaged Reality**: In real-world products, the fundamental distinction is between "engine-managed transient data" and "system-managed persistent data." 

By moving to the **Managed/Unmanaged Pool** model, GEF achieved:
- **Simpler Logic**: Lifetime is either automatic (refcounted) or manual (system-controlled).
- **Better Performance**: The pool can optimize allocations for transient data much more effectively when it knows the data is refcounted.
- **Cleaner API**: The `promote()` function was removed entirely, replaced by engineer-level configuration of the `ResourcePool`.
- **Clearer Personas**: Researchers focus on `Input/Output`, while Engineers focus on which `Pool` those bindings utilize.
