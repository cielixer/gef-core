# Validation Layer

## Introduction

The General Engine Framework (GEF) Validation Layer is a multi-phase safety system designed to detect architectural, logical, and runtime errors as early as possible in the development lifecycle. Given GEF's "researcher-first" philosophy—which prioritizes rapid iteration and hot-reloading—the validation layer serves as the primary safeguard against the complexities of a highly parallel, plugin-based system.

By enforcing strict semantics on data bindings and module compositions, the validation layer ensures that researchers can experiment with complex algorithms without falling into common traps of multi-threaded development, such as race conditions, memory corruption, or dangling references.

### Data Integrity and Parallelism
The core challenge in a data-driven framework like GEF is maintaining data integrity while maximizing parallel execution. The validation layer acts as the "referee," ensuring that the scheduler's attempts to run modules concurrently do not violate the fundamental access rules defined by the binding model (e.g., Single-Writer or Multiple-Readers).

## Design Rationale: The "Shift Left" Principle

The core philosophy of the GEF validation system is to "shift left": moving error detection from runtime to build-time or schedule-time. Detecting a cyclic dependency or a binding conflict at the moment a YAML file is saved is significantly cheaper and more efficient than debugging a deadlock or a segfault during a long-running computation.

Validation is split into three distinct phases to balance thoroughness with performance:
1. **Build-time**: Instant feedback during composition and loading.
2. **Schedule-time**: Algorithmic verification of parallelism safety.
3. **Runtime**: Deep inspection for logical errors (Debug-only).

## Validation Phases

### 1. Build-time Validation (Static)
This phase occurs during YAML parsing and module loading. It analyzes individual modules and their relationships within a Flow or Pipeline. It focuses on the structural integrity of the graph and the validity of binding declarations.

*   **Scope**: Individual modules, module interfaces, and local wiring.
*   **Mechanism**: Static analysis of `extern "C"` metadata and YAML schema.
*   **Performance**: Extremely fast; triggered on hot-reload or initial system boot.

### 2. Schedule-time Validation (DAG Analysis)
Triggered after the Static Scheduler has generated an execution plan but before any modules are dispatched. This phase analyzes the "Parallel Groups" to ensure that the concurrency extracted by the scheduler is actually safe.

*   **Scope**: Parallel execution groups and cross-module data dependencies.
*   **Mechanism**: Graph traversal and intersection analysis of binding sets.
*   **Performance**: Moderate; proportional to the complexity of the DAG and the number of parallelizable branches.

### 3. Runtime Validation (Debug-Only)
The final line of defense, active only in Debug builds. It performs deep inspection of memory access patterns and resource lifetimes. In Release builds, this entire layer is compiled out, ensuring zero overhead for production systems.

*   **Scope**: Resource mutation, pointer safety, and backend compatibility.
*   **Mechanism**: Instrumentation of proxy types (`InOutRef<T>`, etc.) and resource pool tracking.
*   **Performance**: Significant overhead; intended for development and testing only. Not suitable for performance benchmarking.

## Complete Error Type Reference

The following table details all 11 core errors detected by the GEF Validation Layer.

| Error Name | Phase | Description | Trigger Condition | Suggested Fix |
| :--- | :--- | :--- | :--- | :--- |
| **BindingConflict** | Build | Name collision in module interface. | A module declares the same name as both an `Input` and an `Output`. | Rename one of the bindings or use `InOut` if mutation is intended. |
| **UnresolvedBinding** | Build | Broken dataflow link in the DAG. | A module declares an `Input` "X", but no upstream module produces an `Output` or `InOut` named "X". | Check YAML wiring or ensure an upstream module produces the required data. |
| **TypeMismatch** | Build | Incompatible data types between modules. | An `Input` expects `Tensor`, but the upstream `Output` provides a `Scalar`. | Update module code to use compatible types or insert a conversion module. |
| **UnusedOutput** | Build | Redundant data production (Warning). | An `Output` is produced by a module but is not consumed by any downstream module or system sink. | Remove the unused output to save memory, or ignore if the output is for external inspection. |
| **CyclicDependency** | Build | Infinite loop in the module graph. | The module graph contains a path that leads back to a previous module (e.g., A -> B -> A). | Break the cycle by redesigning the flow; use feedback modules if temporal recursion is needed. |
| **InOutRace** | Schedule | Parallel mutation conflict on same data. | Two modules in the same parallel group both declare `InOut` on the same resource. | The scheduler will normally serialize these, but explicit wiring errors can trigger this. |
| **InOutReadRace** | Schedule | Concurrent read/write conflict. | One module has `InOut` while another in the same parallel group has `Input` on the same resource. | Ensure the mutation happens before or after the readers in the DAG by adding a dependency. |
| **BatchOverlap** | Schedule | Data-parallel partition race. | A `Batch` split strategy produces overlapping regions for parallel `InOut` operations. | Examine the `split:` configuration; ensure partitions (e.g., tensor slices) are disjoint. |
| **InOutNoWrite** | Runtime | Lazy researcher pattern (Debug only). | A module declares `InOut` but never actually writes to the resource during execution. | Change the binding to `Input` to allow better parallelism and reduce locking overhead. |
| **UseAfterFree** | Runtime | Dangling resource access (Debug only). | A module attempts to access a managed resource after its reference count has reached zero. | Check ResourcePool configuration or ensure no asynchronous references escape module execution. |
| **BackendMismatch** | Runtime | Hardware-specific access error. | A CPU-based module attempts to read or write data residing in GPU memory. | Insert a host/device transfer module or configure the ResourcePool to use compatible memory. |

## Debug-Only Profiling and Tracking

In addition to error detection, the validation layer provides instrumentation for performance profiling and resource auditing in Debug builds.

### Write Tracking
For `InOut` bindings, the framework tracks whether a write actually occurred. This is achieved by monitoring the "dirty bit" on the underlying resource or, for complex types, performing a shallow checksum of the header. If the module finishes without a write, the `InOutNoWrite` warning is triggered. This encourages researchers to use the most restrictive binding possible, which in turn maximizes the scheduler's ability to parallelize execution.

### Resource Lifetime Visualization
The validation layer tracks the birth and death of every resource in the `Managed ResourcePool`. This allows engineers to visualize memory pressure over the course of a Flow's execution and identify "memory hogs" that hold onto large buffers longer than necessary. 

### Zero-Overhead Strategy
GEF uses a "Zero-Cost Release" strategy. All validation checks that require metadata tracking or memory inspection are wrapped in `GEF_ENABLE_VALIDATION` macros. In Release builds, these macros expand to nothing, and the proxy types (e.g., `InputRef<T>`) resolve to simple pointers or references with no internal tracking logic.

## Relationship to the Scheduler

The Validation Layer and the Scheduler are tightly coupled but logically distinct. This separation allows the scheduler to focus on performance while the validation layer focuses on correctness.

1.  **Plan Generation**: The **Static Scheduler** proposes an execution plan (a sequence of Parallel Groups) based on data dependencies.
2.  **Audit**: The **Validation Layer** audits this plan. If an `InOutRace` or `InOutReadRace` is detected, the validation layer rejects the plan and prevents execution, throwing a `ScheduleError`.
3.  **Execution Feedback**: The **Dynamic Scheduler** executes the plan and, in Debug mode, reports runtime violations (like `UseAfterFree`) back to the validation layer for logging.

## Error Reporting and Interpretation

GEF aims for "compiler-grade" error messages. When a validation error occurs, the system provides a structured report:
*   **Error Code**: A unique identifier (e.g., `GEF_VAL_005`) for looking up detailed documentation.
*   **Context**: The name of the Flow/Pipeline and the specific modules (and their instance names) involved.
*   **Trace**: For graph errors like `CyclicDependency`, an ASCII representation of the problematic path.
*   **Suggestion**: A human-readable suggestion on how to resolve the issue.

Example Error Output:
```text
[GEF VALIDATION ERROR] UnresolvedBinding in 'MainFlow'
Module: 'ThresholdFilter' (instance: 'thresh_1')
Error: Input 'mask' has no provider upstream.
Upstream modules in this flow produce: ['raw_image', 'blurred_image', 'metadata']
Suggested Fix: Check if 'mask' was renamed in the source module or if a 'MaskGenerator' is missing from the YAML.
```

## Implementation Notes: Resource Pool Integration

The validation layer integrates directly with the `ResourcePool` to track resource ownership. In Debug mode, every `Resource` object maintains a `ValidationMetadata` structure. This structure tracks:
- The last module to write to the resource.
- The list of modules currently reading the resource.
- The timestamp of the last mutation.

When a module calls `ctx.input<T>()`, the validation layer checks if any other module currently has an `InOut` borrow on that resource. If so, and if those modules are in the same parallel group, a race is detected.

## Design Rationale: Why 3 Phases?

The 3-phase approach is a direct response to the requirements of high-performance compute frameworks:

*   **Build-time** is for the **Researcher**. It catches typos and structural logic errors during the YAML-writing phase. It must be instant to support the "save and run" Flow.
*   **Schedule-time** is for the **Framework**. It catches subtle concurrency bugs that are invisible in the YAML (which only shows connections) but emerge when the scheduler tries to optimize execution.
*   **Runtime** is for the **Engineer**. It catches the "hard" bugs—memory leaks, hardware mismatches, and logic flaws that only manifest with specific runtime data or edge cases.

By separating these, GEF ensures that the most common errors are caught the fastest, while the most complex errors are caught eventually, all without slowing down the final production system's throughput.

## Future Directions: Extensible Validation

As GEF evolves, the validation layer is designed to support custom, domain-specific rules. For example, a Computer Vision extension might add a validation rule to ensure that all images in a Flow share the same color space or resolution before they reach a merge module. These custom rules will hook into the Build-time phase, allowing researchers to catch domain-specific logical errors without waiting for runtime execution.
