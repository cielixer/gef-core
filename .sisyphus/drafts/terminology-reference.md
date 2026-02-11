# GEF Terminology Reference

**Authoritative terminology extracted from design interview rounds 1-11.**
**Last updated: Interview Round 11**

---

## Binding Kinds

Four fundamental binding types define how data flows between modules. These replace the earlier experimental 4-kind model (Config/State/View/Variable) from Round 1.

### Input
- **Definition**: Read-only incoming data. Module cannot modify this data.
- **Semantics**: Multiple modules can read the same Input concurrently (safe parallelism).
- **Use case**: Pass data into a module for processing without modification.
- **Lifetime**: Determined by ResourcePool (managed/unmanaged).

### Output
- **Definition**: New data produced by this module. Always a new resource.
- **Semantics**: Each module output is unique; no conflicts with other modules.
- **Use case**: Produce new computed data.
- **Conflict rule**: A module cannot use the same name as both Input and Output (build-time error).

### InOut
- **Definition**: Receives data AND modifies it in-place. Explicit mutation binding.
- **Semantics**: Exclusive access required. Only one module can InOut the same data at a time. Serializes against readers/writers of the same data.
- **Use case**: Large tensors, images, video — where in-place mutation saves memory.
- **Debug guard**: DEBUG MODE ONLY: If InOut declared but no write detected, warning is issued. Zero overhead in release builds.

### Config
- **Definition**: Module-scoped, immutable, engine-lifetime setting.
- **Semantics**: Flat namespace, injected per module. Multiple modules can access the same Config concurrently (immutable).
- **Scope**: Global (engine-lifetime, accessible within a module execution).
- **Use case**: Algorithm parameters, thresholds, configuration options.
- **Lifecycle**: Persists for the entire execution (managed by System, not ResourcePool).

---

## Module Types

Four types of modules compose into larger systems. All are **static** — structure known at build time.

### Atomic
- **Definition**: Single computation unit. Compiled as a .cpp file → shared library (.so/.dylib).
- **Structure**: One file, one module, no composition.
- **Hot-reload**: Modify one .cpp file → rebuild only that .so → immediately runnable.
- **Interface**: Declares Input/Output/InOut/Config bindings explicitly.
- **Example**: Image blur filter, tensor transpose.

### Pipeline
- **Definition**: Sequential chain of modules. Executes in order.
- **Structure**: Contains `modules:` (child modules). No parallelism between stages.
- **Semantics**: Variable list semantics. Outputs matched to inputs by name. Skip-connections allowed (A's output can reach C even if B doesn't use it).
- **Interface**: Declares `inputs:`, `outputs:`, `configs:`.
- **Example**: read image → blur → threshold → save.

### Flow
- **Definition**: DAG (directed acyclic graph) of modules. Parallelism where possible.
- **Structure**: Contains `modules:` (child modules). Scheduler determines parallelizable groups.
- **Semantics**: Explicit wiring required for complex patterns. Arrow syntax: `blur.blurred -> threshold.image` or structured `{from: blur.blurred, to: threshold.image}`.
- **Interface**: Declares `inputs:`, `outputs:`, `configs:`.
- **Example**: Read image → (blur + denoise in parallel) → merge → output.

### Batch
- **Definition**: Wrapper module. Runs an inner module in parallel over a collection.
- **Transparent**: Inner module doesn't know it's batched. Sees single element.
- **Structure**: Contains `inner:`, `split:`, `gather:`. Defines how data is partitioned and recombined.
- **Split strategies**: `batch_tensor_dim` (split tensor along a dimension), `batch_sequence` (split List<T> into elements), custom split module.
- **Gather**: Mirrors split strategy. Handles output assembly.
- **Race detection**: ERROR on overlapping InOut. Build-time detection impossible due to variable data sizes (runtime detection by Gather module).
- **Example**: Batch process video frames: one frame per parallel task.

---

## YAML Configuration Fields

Authoritative field names used in module configuration files (Rounds 5, 6).

### Core Module Fields
- **`modules:`** — List of child modules (Atomic modules referenced, Pipeline/Flow/Batch defined inline). NOT "stages".
- **`inputs:`** — Declare which inputs this Pipeline/Flow/Batch exposes.
- **`outputs:`** — Declare which outputs this Pipeline/Flow/Batch produces.
- **`configs:`** — Declare Config bindings. Flat namespace. Shared with child modules.
- **`wiring:`** — Explicit edge definitions when implicit name-matching is insufficient.

### Batch-Specific Fields
- **`split:`** — Strategy and configuration for partitioning input data.
  - Contains: `strategy` (batch_tensor_dim, batch_sequence, custom) + strategy-specific config.
- **`gather:`** — Strategy and configuration for recombining output data.
  - Contains: `strategy` (mirrors split) + config.
- **`inner:`** — Reference to the wrapped module.
- **`mapping:`** — Maps split elements to inner inputs, inner outputs to gather.

### Pipeline/Flow Shared Fields
- **`type:`** — Either "pipeline" or "flow". Determines execution semantics.
- **`name:`** — Module identifier.

---

## C++ API Macros

Macro-based declaration system for Atomic modules (Round 5).

### Module Declaration
- **`GEF_MODULE`** — Declares an Atomic module. Usage: `GEF_MODULE(ModuleName) { ... }`

### Binding Macros
- **`GEF_INPUT(Type, name)`** — Declare an Input binding.
- **`GEF_OUTPUT(Type, name)`** — Declare an Output binding.
- **`GEF_INOUT(Type, name)`** — Declare an InOut binding.
- **`GEF_CONFIG(Type, name)`** — Declare a Config binding.

### Example
```cpp
GEF_MODULE(BlurFilter) {
  GEF_INPUT(Image, input_image);
  GEF_OUTPUT(Image, blurred_image);
  GEF_CONFIG(int, kernel_size);
  
  // execute() method defined inside
}
```

---

## Context API Methods

Methods available on the Context parameter passed to module execute() (Round 5).

### Data Access
- **`ctx.input<T>()`** — Retrieve read-only Input binding of type T.
- **`ctx.output<T>()`** — Retrieve Output binding of type T (mutable reference for writing).
- **`ctx.inout<T>()`** — Retrieve InOut binding of type T (mutable reference for modification).
- **`ctx.config<T>()`** — Retrieve Config binding of type T (read-only, immutable).

### Proxy Types
- Return types are proxy types (InputRef<T>, InOutRef<T>) to ensure `auto` doesn't accidentally copy.
- Proxy types behave like references, safe with auto deduction.
- Prevents unintended memory copies on large tensors/images/video.

### Example
```cpp
auto input_img = ctx.input<Image>();    // InputRef<Image>
auto output_img = ctx.output<Image>();  // returns mutable reference
auto kernel = ctx.config<int>();        // const ref
```

---

## Banned Terminology

Terms from earlier design iterations that are **NOT** used in current GEF design. Explanation for each removal.

### State (as binding)
- **Why removed**: Redundant. Config (immutable engine-level) + InOut (explicit mutation) cover all use cases.
- **Replaced by**: Config for read-only settings, InOut for explicit in-place mutation.

### View (as binding)
- **Why removed**: Created ambiguity in binding tracking. A View input → same-name output pattern made lineage unclear.
- **Replaced by**: Input (read-only) and Output (new data) with build-time error if same name used as both.

### Variable (as binding)
- **Why removed**: Too generic. Encouraged lazy researcher patterns (everything becomes Variable). Three-binding model (Input/Output/InOut) is explicit and safe.
- **Replaced by**: Input, Output, InOut — each with clear semantics.

### Workflow
- **Why removed**: Naming inconsistency. "Workflow" is ambiguous and often means different things in different systems.
- **Replaced by**: Flow (DAG with parallelism).

### Consistent Scope (old scope model)
- **Why removed**: Scope model replaced entirely in Round 7. ResourcePool abstraction (managed/unmanaged) replaces Module/Consistent/Global scope hierarchy.
- **Replaced by**: Managed ResourcePool (automatic reference-counting, transient) and Unmanaged ResourcePool (System-controlled, persistent).

### promote()
- **Why removed**: Was a mechanism for moving data between scopes in old model. No longer needed with managed/unmanaged pool abstraction.
- **Replaced by**: System configures which ResourcePool each resource uses.

### `stages:` (YAML field)
- **Why removed**: Inconsistent terminology. Modules are called "modules", not "stages".
- **Replaced by**: `modules:` field in YAML.

---

## Running Example Domain

**Image Processing Pipeline**

All tutorial examples, code samples, and case studies use image processing as the running domain.

### Domain characteristics:
- **Familiar**: Researchers understand images, transformations, effects.
- **Scalable**: From simple (blur, threshold) to complex (detection, reconstruction).
- **Real-time**: Video processing demonstrates parallelism and batching naturally.
- **Memory-intensive**: Shows InOut binding, resource pool, and performance considerations.

### Example pipeline:
1. **Read**: Load image from file (Atomic)
2. **Enhance**: Blur + denoise in parallel (Flow)
3. **Detect**: Edge detection, thresholding (Pipeline)
4. **Batch**: Process video frame-by-frame in parallel (Batch)
5. **Output**: Save result (Atomic)

### Data types in examples:
- `Image` — 2D raster (rows, cols, channels)
- `Tensor<T, Dims>` — Multi-dimensional array
- `Scalar` — Single value
- `Region` — Bounding box or ROI
- `Metadata` — Annotations, labels

---

## ResourcePool Model (Round 7)

Two strategies for managing resource lifetime and ownership.

### Managed ResourcePool
- **Behavior**: Reference-counted. Automatic release when refcount reaches 0.
- **Use case**: Transient data flowing within a single execution.
- **Scope**: Single execution. Data lifetime tied to execution scope.
- **Overhead**: Minimal — only refcount increments/decrements.
- **Example**: Intermediate blur result flowing between modules.

### Unmanaged ResourcePool
- **Behavior**: No refcount. System manually allocates and releases.
- **Use case**: Persistent data across multiple executions or framework lifetime.
- **Scope**: System-controlled lifetime. Engineer decides lifecycle.
- **Overhead**: Zero — no refcount tracking.
- **Example**: Model weights, lookup tables, persistent caches.

### Module Access
- Modules access data through same Input/Output/InOut bindings regardless of pool type.
- System (engineer) configures which resources come from which pool.
- Advanced: Modules can specify which ResourcePool to use.

---

## System & Scheduler (Rounds 7-8)

### System
- **Definition**: Top-level program. Single instance per execution.
- **Responsibilities**: Load modules, run scheduler, manage execution lifecycle, own ResourcePool(s).
- **Final product**: Game engine, video processor, backend server, etc.
- **Config**: ResourcePool strategy, scheduling strategy, execution parameters.

### Scheduler (Two Levels)
- **Static Scheduler**: Analyzes Input/Output/InOut declarations. Builds DAG. Pre-computes parallelizable groups.
- **Dynamic Scheduler**: Runs on top of static scheduler. Handles runtime decisions (branching, batching). Invokes static scheduler for sub-graphs.
- **Developer choice**: Engineer specifies which scheduler to use, or implements custom scheduler.

### Module Discovery
- Directory scan for .so/.dylib files.
- Plugin metadata via `extern "C" gef_get_metadata()` function.
- Macros generate this automatically.

---

## Validation Errors (Round 10)

### Build-time errors (Static validation)
- **BindingConflict**: Same name declared as both Input and Output in one module.
- **UnresolvedBinding**: Module declares input "X" but no upstream module produces it.
- **TypeMismatch**: Input expects Tensor but upstream produces Scalar.
- **CyclicDependency**: Module graph contains a cycle.

### Build-time warnings
- **UnusedOutput**: Output that no downstream module consumes.

### Schedule-time checks (DAG analysis)
- **InOutRace**: Two parallel modules both InOut the same data.
- **InOutReadRace**: InOut and Input on same data in same parallel group.
- **BatchOverlap**: Batch split produces overlapping InOut regions.

### Runtime checks (Execution-time, debug-only)
- **InOutNoWrite**: InOut declared but never written. Should be Input. Debug-only, zero overhead in release.
- **UseAfterFree**: Accessing released managed pool resource.
- **BackendMismatch**: CPU module accesses GPU data (future).

---

## Two User Personas

### Researcher
- **Role**: Algorithm development, module authorship.
- **Touches**: Module .cpp files, YAML composition configs.
- **Focus**: Writes Atomic modules, composes Pipeline/Flow/Batch via YAML.
- **Books**: Tutorial (docs/guide/).

### Engineer
- **Role**: System development, deployment, infrastructure.
- **Touches**: System config, ResourcePool strategy, scheduling, persistence.
- **Focus**: Configures GEF runtime, integrates into product, manages resources.
- **Books**: Architecture Reference (docs/design/).

---

## Two Documentation Books

### Tutorial (docs/guide/)
- **Audience**: Researchers. "How to use GEF."
- **Style**: Hands-on, example-driven, progressive difficulty.
- **Examples**: Working, compilable C++ code (once GEF is built).
- **TOC**: 00-introduction, 01-quick-start, 02-your-first-module, 03-bindings, 04-composing-pipelines, 05-flows-and-parallelism, 06-batch-processing, 07-resource-management, 08-debugging, 09-external-libraries, 10-example-projects.

### Architecture Reference (docs/design/)
- **Audience**: Engineers. "How GEF works."
- **Style**: Hub + deep-dive files, design rationale, technical depth.
- **Diagrams**: ASCII art only.
- **TOC**: overview, system, module-system, binding-model, resource-pool, scheduler, validation-layer, yaml-config, plugin-architecture, external-adapters, error-handling, development-roadmap.

---

## Design Philosophy

Drawn from decision patterns across all 11 interview rounds.

### Core Principles
1. **Researcher-first DX**: Hot-reload one file, run immediately. No build systems for researchers.
2. **Explicit over implicit**: Binding kinds have clear semantics. Same-name Input+Output is a build error.
3. **Memory efficiency**: InOut for in-place mutation (large tensors). Managed/Unmanaged pools balance safety and performance.
4. **Parallelism from declarations**: Scheduler extracts parallelism from Input/Output/InOut bindings automatically.
5. **Safety with zero overhead**: Debug-only checks (InOut write detection). Release builds have no overhead.
6. **Composition over inheritance**: Modules composed via YAML wiring, not OOP hierarchy.

### Writing Style (Rust Book style)
- Technical but accessible.
- Progressive: simple examples → complex patterns.
- Clear explanations of "why", not just "how".
- Real, compilable code examples.
- ASCII diagrams for visualization.

---

## File and Version Reference

- **Design source**: `.sisyphus/drafts/gef-book-design.md` (11 interview rounds)
- **Latest round**: Round 11 (Book structure decisions)
- **Terminology freeze**: This document is authoritative. All subsequent documentation uses these terms exactly as defined.

