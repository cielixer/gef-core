# Binding Model

## Overview

The binding model is the core contract between a GEF module and the framework's data system. Bindings define how a module accesses, produces, and modifies data, serving as the interface through which the scheduler determines execution order and parallelism.

In GEF, modules do not own data; they declare **bindings** to data that is managed by the system's `ResourcePool`. These declarations are expressive enough to convey the module's intent (reading, writing, or mutating) while remaining simple enough for researchers to define without deep knowledge of the underlying concurrency model.

By decoupling data access from data ownership, the binding model enables:
- **Automatic Parallelization**: The scheduler derives a dependency graph purely from binding declarations.
- **Memory Efficiency**: Explicit in-place mutation (InOut) prevents unnecessary copies of large buffers.
- **Safety**: Build-time and runtime checks prevent race conditions and data corruption.

## Design Evolution

The binding model underwent a complete redesign to address ambiguities and safety concerns found in the initial experimental design.

### The Old Model: Config, State, View, Variable
Initially, GEF used a four-kind model:
- **Config**: Immutable, module-scoped settings.
- **State**: Mutable, module-scoped persistent data.
- **View**: Immutable dataflow (inputs).
- **Variable**: Mutable dataflow (outputs/in-place).

### Problems Identified
1. **Tracking Ambiguity**: A common pattern was for a module to receive a `View` input and produce an `Output` (Variable) with the same name but different data (e.g., a "blurred" image replacing an "original" image). This made data lineage tracking difficult for the scheduler and validation layer.
2. **Researcher Laziness**: Because `Variable` was a catch-all for mutable data, researchers tended to use it for everything, bypassing the safety and parallelism benefits of more specific bindings.
3. **State Redundancy**: `State` was intended for per-module persistent data, but it overlapped significantly with `Config` (for immutable settings) and `InOut` (for mutable data).

### The New Model: Input, Output, InOut, Config
The new model simplifies the taxonomy into four precise kinds: `Input`, `Output`, `InOut`, and `Config`. This change enforces a "one-intent-per-binding" rule, where the researcher must explicitly choose between reading, producing, or mutating.

## The Four Binding Kinds

### 1. Input
- **Definition**: Read-only incoming data from an upstream module or the system.
- **Semantics**: Const reference semantics. The module is guaranteed that the data will not change during its execution.
- **Concurrency**: Highly parallelizable. Multiple modules can read the same `Input` simultaneously without synchronization.
- **Safety**: Enforced by the framework via `InputRef<T>` proxy types that prevent modification.

### 2. Output
- **Definition**: New data produced by the module.
- **Semantics**: Always results in the allocation (or reuse from a pool) of a **new resource**.
- **Concurrency**: Independent of other modules using the same name as an `Input`, as it produces a fresh data instance.
- **Safety**: Prevents accidental overwriting of shared inputs.

### 3. InOut
- **Definition**: Receives existing data and modifies it in-place.
- **Semantics**: Explicit mutation binding. It represents a "borrow-for-write" from the `ResourcePool`.
- **Concurrency**: Requires **exclusive access**. The scheduler ensures that no other module is reading or writing to this specific resource instance while the module is executing.
- **Rationale**: Essential for high-performance processing of large tensors, video frames, or GPU buffers where copying is prohibitively expensive.

### 4. Config
- **Definition**: Immutable, engine-lifetime settings injected into the module.
- **Semantics**: Flat namespace injection. Unlike `Input`, which flows through the DAG, `Config` values are typically global or semi-static parameters.
- **Concurrency**: Always safe for concurrent access across any number of modules.
- **Lifecycle**: Managed by the `System`, persisting for the duration of the application execution.

## The Same-Name Rule

A critical safety feature of the new binding model is the **Same-Name Rule**:

> A module cannot declare a binding with the same name as both an `Input` and an `Output`.

If a module attempts to do this, the validation layer generates a **BUILD ERROR**.

**Rationale**: This rule entirely eliminates the tracking ambiguity present in the old model. In the old model, a researcher might take an image named "buffer" as a `View` and output a modified version also named "buffer" as a `Variable`. In the new model, they must either:
1. Use `InOut(buffer)` to indicate they are mutating the buffer in-place.
2. Use `Input(buffer)` and `Output(processed_buffer)` to indicate they are producing new data.

This clarity allows the scheduler to build a deterministic and unambiguous data lineage.

## Parallelism Derivation

The scheduler determines the maximum possible parallelism by analyzing the binding declarations of all modules in a `Flow`.

| Binding | Access Intent | Parallelism Semantics |
|---------|---------------|-----------------------|
| **Input** | Read-Only | Safe to parallelize; multiple readers allowed concurrently. |
| **Output** | Write (New) | Safe; produces a new resource, no conflict with existing data. |
| **InOut** | Read + Write | **Serializes** against any other reader or writer of the same data. |
| **Config** | Read-Only | Always safe; immutable for the lifetime of the engine. |

By adhering to these rules, the scheduler can automatically group modules into parallel execution buckets without the developer needing to write mutexes or semaphores.

## InOut Exclusive Access

While `Input` and `Output` provide the safest dataflow, `InOut` is provided for performance-critical sections.

### Why it is needed
In many research domains (Deep Learning, Real-time Computer Vision), data structures like 4K video frames or multi-gigabyte tensors are common. Allocating a new resource for every minor transformation would lead to:
1. Excessive memory pressure and OOM errors.
2. Significant latency from allocation and deallocation overhead.
3. Cache misses from constantly moving to new memory addresses.

### How it works
When a module requests an `InOut` binding, the framework requests an **exclusive borrow** from the `ResourcePool`. If the resource is currently being read as an `Input` by another parallel module, the scheduler will delay the execution of the `InOut` module until all readers have finished. This "Single Writer OR Multiple Readers" (SWMR) pattern is enforced at the framework level.

## Config Flat Namespace

Config bindings operate in a flat namespace managed by the `System`.

### Injection Mechanism
Modules declare the specific config keys they require. During module initialization, the `System` injects the corresponding values from the YAML configuration file or the global environment.

### Disambiguation
In the event of name collisions (e.g., two different modules both needing a key named `threshold` with different values), the `System` provides a mechanism for **qualified names**. In the YAML configuration, the engineer can map specific module instances to distinct config paths, ensuring each module receives the correct parameters while maintaining a simple internal declaration.

## Debug-Mode InOut Write Detection

To prevent the "Lazy Researcher" anti-pattern (declaring everything as `InOut` to avoid thinking about dataflow), GEF includes a specialized validation check.

### Mechanism
In **Debug builds**, the framework tracks the memory pages or resource handles associated with an `InOut` binding. If a module completes its execution and the framework detects that no modifications were actually made to the data, a warning is issued:
> "Warning: Module 'X' declared InOut('Y') but performed no writes. Consider using Input('Y') for better parallelism."

### Rationale
This check is computationally expensive as it requires tracking resource state or checksums. Therefore, it is strictly limited to Debug mode and is compiled out of Release builds to ensure zero overhead in production.

## Proxy Types Design Rationale

GEF uses proxy types—`InputRef<T>`, `OutputRef<T>`, and `InOutRef<T>`—as the return types for the `Context` data access methods.

### Auto Safety
In modern C++, researchers frequently use `auto` to capture return values. If `ctx.input<Tensor>()` returned a raw `Tensor` or a standard reference, a researcher might accidentally write:
```cpp
auto my_data = ctx.input<Tensor>(); // Might copy the tensor!
```
By returning a proxy type, GEF ensures that even with `auto`, the researcher is holding a lightweight handle that behaves like a reference but cannot trigger an accidental deep copy. The proxy types also allow the framework to inject instrumentation for the debug-mode write detection and borrow checking.

## State Removal Rationale

The removal of the `State` binding kind was a deliberate simplification of the architecture.

In early designs, `State` was intended to solve the problem of "persistent mutable data" belonging to a module (e.g., an internal counter or a running average). However, it was realized that:
1. **Config** + **Unmanaged ResourcePool** covers this: A module can receive a handle to a persistent resource from the unmanaged pool via a `Config` setting, and then use `InOut` to mutate it.
2. **Side Effects**: `State` often introduced hidden side-effects that made modules difficult to test and parallelize. By forcing all mutable data through `InOut`, the dependencies become explicit in the DAG, allowing the scheduler to maintain safety.

## Alternatives Considered

### 1. Pure Functional Model
We considered a model where all modules are functionally pure: `Inputs` in, `Outputs` out, no mutation.
- **Pros**: Perfectly safe, trivial to parallelize.
- **Cons**: Disastrous for memory-intensive research. Copying a 1GB tensor to change one value is unacceptable.

### 2. Auto-Allocation
We considered a model where the framework automatically decides when to copy and when to mutate.
- **Pros**: Simplest for the researcher.
- **Cons**: Non-deterministic performance. Small changes in module order could trigger massive hidden copies, leading to "performance cliffs" that are hard for researchers to debug.

GEF chose the **Explicit Intent** model (InOut vs Output) as it provides the best balance of safety, performance, and transparency.

## Relationship to ResourcePool

Bindings are ultimately **views** into resources managed by the `ResourcePool`.

- When a module defines an `Input`, it is requesting a **shared read-only view**.
- When a module defines an `Output`, it is requesting the pool to **instantiate a new resource view**.
- When a module defines an `InOut`, it is requesting an **exclusive mutable view**.

The binding model acts as the "Frontend" (declaration of intent), while the `ResourcePool` acts as the "Backend" (fulfillment of memory/handles). This separation allows GEF to run the same module code on different hardware backends (CPU, GPU, DSP) simply by swapping the underlying `ResourcePool` implementation, without changing the module's binding declarations.
