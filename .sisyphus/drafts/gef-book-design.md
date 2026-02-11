# Draft: GEF Design Book / Tutorial Document

## What This Is
Interactive interview to extract the full GEF design from the user's mind, then produce a comprehensive book-like document under `docs/`.

## Requirements (confirmed)
- Output: A book/tutorial-style document consolidating GEF's complete design
- Location: Under `docs/` directory
- No code work — documentation only
- Build system: pixi + C++23 (user has had trouble configuring C++23)
- Target audience: Researchers who want to develop code and run it immediately

## What Already Exists (6 design docs)
1. **architecture.md** — High-level overview: core concepts (Module, Variable, Config, State, Resource, Binding, Scope), data/memory architecture, execution model, external library integration, validation layer, resource state machine, development phases, philosophy
2. **module_system.md** — Stage/Pipeline/Flow composition model, variable list semantics, DAG semantics
3. **data_system.md** — Bindings (Config/State/View/Variable), scope model, concurrency/parallelism rules, State safety, immutability preference
4. **validation_layer.md** — Role, timing (static/dynamic), error types, logging/profiling
5. **scheduler.md** — Goals, inputs, processing steps, outputs
6. **resource_pool.md** — Ownership, scope, borrow, FSM, API, zero-copy views, backend

## Observations / Gaps Noticed
- architecture.md mentions "Workflow" but module_system.md calls it "Flow" — naming inconsistency?
- data_system.md refined the binding model: Config/State/View/Variable (4 kinds) vs architecture.md which mentions Config/Variable/State (3 roles) — evolution of design
- The "Scope" model evolved: architecture.md has Module/Consistent/Global scopes, but data_system.md introduces "declared set" visibility for Variable/View — more nuanced
- No document covers: error handling philosophy, logging system, plugin/extension model, build system details, API surface for end users (researcher experience), configuration file format, example use cases
- The "researcher-first" goal needs more fleshing out: what does the DX look like?
- External library integration (ArrayFire, OpenCV, Eigen) is mentioned but adapter design isn't detailed
- No document on threading model specifics (thread pool size, work stealing, etc.)

## Technical Decisions (from Interview Round 1)
- **Final naming**: Flow (not Workflow). Confirmed.
- **Module definition**: NOT inheritance-based. User has OOP aversion. Each Stage = a single .cpp file compiled as a shared library (plugin model).
- **Hot-reload model**: Researcher modifies/creates ONE .cpp file → only that .so is rebuilt → program is immediately runnable. This implies a dynamic loading / plugin architecture.
- **Binding kinds**: 4-kind model (Config/State/View/Variable) is the starting point but user identified a problem: when a Module receives a View input and outputs with the same name but different data, tracking becomes ambiguous. NEEDS RESOLUTION.

## Key Design Insight: Plugin Architecture
- Each Stage is a .cpp file → shared library (.so/.dylib)
- Only the modified file is recompiled
- The engine dynamically loads these
- This is the "researcher-friendly" core: modify one file, rebuild one .so, run immediately
- This is closer to a plugin system than traditional framework usage

## Open Questions (remaining)
- BINDING REDESIGN: User is stuck on binding kinds. Current 4-kind model has issues:
  1. Researchers may lazily set everything as Variable (safety concern)
  2. View input → same-name output creates tracking ambiguity
  3. User wants to redesign binding kinds entirely — needs help
- Error handling philosophy: exceptions? Result types? Error codes?
- What external libraries are actually planned vs aspirational?
- Threading model details?
- Book structure preferences?
- GUI tool for module composition (future vision)

## Interview Round 5 Decisions
- **Config scoping**: Flat namespace, injected per Module. No hierarchy. Each Module declares what Config keys it needs.
- **External libraries**: Open adapter pattern, start minimal with Eigen. Grow organically.
- **Threading model**: Abstract — scheduler decides. Framework doesn't prescribe fixed/work-stealing.
- **Config file format**: User wants a standard format first (TOML/YAML/JSON), custom DSL later. Needs recommendation for DAG expression.

### Config File Format Analysis (for DAG expression)
User wants to express a DAG (nodes + edges) in the config file. Comparing:
- **TOML**: Good for flat config. POOR for DAG expression — nested arrays of tables are awkward for graph edges. Not ideal.
- **YAML**: Better for nested/complex structures. Can express edges naturally. But whitespace-sensitive parsing.
- **JSON**: Universal but extremely verbose for humans. Bad DX for researchers.
- **Recommendation**: YAML is best for DAG expression among standard formats. Edges can be expressed as lists, nodes as mappings. Future custom DSL can improve DX further.
- **Stage API style**: Macros (Option A) for declaration + execute(Context&). Macros: GEF_MODULE, GEF_INPUT, GEF_OUTPUT, GEF_INOUT, GEF_CONFIG.
- **Context API**: ctx.input<T>(), ctx.output<T>(), ctx.inout<T>(), ctx.config<T>()
- **Auto safety**: Return proxy types (InputRef<T>, InOutRef<T>) so `auto` doesn't accidentally copy. Proxy types behave like references but are safe with auto deduction.
- **Plugin metadata**: Query function approach (extern "C" gef_get_metadata()). Macros generate this behind the scenes.
- **Module discovery**: Directory scan for .so/.dylib files.
- **GPU priority**: CPU-only first, GPU later as extension.
- **Graph definition**: YAML config file for module order and flow edges. Future: GUI tool for visual composition.
- **Pipeline/Flow config**: Composite modules compose from inner modules. Config generated from contained modules.

## Binding Redesign Problem Statement
Core tensions:
1. Safety: prevent researchers from being lazy (everything as Variable)
2. Tracking: same-name input/output with different data breaks lineage
3. Simplicity: researcher-facing API must be simple
4. Expressiveness: must handle real compute patterns (read-only pass-through, in-place mutation, produce new data)

### RESOLVED: New Binding Model (Interview Round 3)
**Dataflow bindings** (between Modules):
- **Input**: Read-only incoming data. Module cannot modify.
- **Output**: New data this Module produces. Always a new resource.
- **InOut**: Receives data AND modifies it in-place. Explicit mutation.
- **RULE**: Same name used in both Input and Output → BUILD ERROR. Prevents ambiguity.

**Engine-level bindings**:
- **Config**: Module-scoped, immutable. Global settings accessible within a module execution.
- ~~**State**: REMOVED~~ — Config + InOut covers all use cases. State was redundant.

**Lazy researcher guard** (Interview Round 4):
- DEBUG MODE ONLY: If InOut is declared but no write is detected → warning
- Rationale: write detection on InOut is computationally expensive, so only in debug builds
- Release mode: no overhead from these checks

**Error handling** (Interview Round 4):
- Hybrid: exceptions for truly unrecoverable (OOM, catastrophic), Result types for expected errors (validation, borrow violations, scope errors)

**Key design choices**:
- In-place mutation is explicitly supported (InOut) — required for large tensors/images/video
- No auto-new-allocation or functional purity — too memory-hungry for target use cases
- Same-name Input+Output is a build-time error — prevents the tracking ambiguity entirely
- This separates intent clearly: "I read" vs "I produce" vs "I mutate"
- Only 3 binding kinds now: Input, Output, InOut (+ Config for engine settings)

**Parallelism implications**:
- Input: multiple Modules can read concurrently → safe
- Output: new data, no conflict → safe
- InOut: exclusive access needed → serializes against other readers/writers of same data
- Config: immutable → always safe
- The scheduler can determine parallelism purely from binding declarations

## Interview Round 6: Pipeline/Flow YAML Config
- **Unified format**: Pipeline and Flow use same YAML structure. `type: pipeline` vs `type: flow`.
- **Mandatory interface**: Every Pipeline/Flow MUST declare `inputs:`, `outputs:`, `configs:` explicitly.
- **Child modules field**: Named `modules:` (NOT "stages" — user wants consistent terminology).
- **Implicit wiring**: Outputs matched to inputs by name. Pipeline supports skip-connections (A's output can reach C even if B doesn't use it).
- **Explicit wiring override**: Arrow syntax (`blur.blurred -> threshold.image`) or structured format (`{from: blur.blurred, to: threshold.image}`).
- **Interface reference**: `inputs.image` and `outputs.processed` — auto-generated invisible modules for input/output boundary.
- **Name collision (Pipeline)**: Last-writer-wins with warning.
- **Name collision (Flow)**: Error unless explicit wiring provided.
- **Multi-file composition**: `include: other.yaml` for referencing sub-modules.
- **Config inheritance**: Pipeline/Flow can declare `configs:` shared with child modules (flat namespace). If names differ, manual wiring needed.
- **Terminology**: No component called "stage" anywhere. Everything is "module" (atomic or composite).

## Interview Round 7: System & ResourcePool
### System Concept
- **System** = the top-level program. There is no higher concept.
- System owns ResourcePool(s), loads modules, runs scheduler, manages execution lifecycle.
- System IS the final product: a game engine, video processor, backend server, etc.
- GEF will provide System examples (game, real-time video, backend server) as references.

### Two User Personas
| Role | Responsibility | Touches |
|---|---|---|
| Researcher | Algorithm development, writes modules (.cpp), composes Pipeline/Flow (YAML) | Module .cpp files, YAML configs |
| Engineer | Builds the System (product). Configures ResourcePools, scheduling, persistence. | System config, ResourcePool, scheduling |

### ResourcePool Redesign
- **Managed ResourcePool**: Reference-counted, automatic release when refcount=0. For transient data flowing within a single execution.
- **Unmanaged ResourcePool**: No refcount, System manually allocates/releases. For persistent data across executions.
- Modules access data through same Input/Output/InOut bindings regardless of pool type.
- System configures which resources come from which pool.
- There should also be a manual way for modules to specify which ResourcePool to use (advanced use case).

### Scope Model: REPLACED
- Old 3-scope model (Module/Consistent/Global) is REPLACED by:
  - Managed pool → automatic lifetime (transient)
  - Unmanaged pool → System-controlled lifetime (persistent)
- Config is always engine-lifetime (effectively "Global")

## Interview Round 8: Scheduler, Validation, Profiling

### Scheduler Architecture (TWO LEVELS)
- **Static Scheduler**: Analyzes Input/Output/InOut declarations, builds DAG, determines parallelizable groups. Pre-computed before execution.
- **Dynamic Scheduler**: Runs on top of static scheduler. Handles runtime decisions like branching (if) and batching (for). Invokes the static scheduler for sub-graphs.
- The developer (engineer) specifies which scheduler to use, or can make their own.
- GEF's goal: provide a universal general-purpose scheduler that handles both static and dynamic aspects.

### Special Module Types (NEW)
1. **Branch (if)**: Routes data to different sub-modules based on runtime conditions. This REQUIRES dynamic scheduling because the path is unknown at build time.
2. **Batch (for)**: Same module runs in parallel on different data items (e.g., each frame of a video processed independently). Data-parallel execution.
- These are NOT designed yet but are part of the vision.

### Validation Layer
- Needs updates for the new binding model. Error types need rethinking.
- ScopeError may be obsolete (scopes were replaced by managed/unmanaged pools).
- BorrowViolation still relevant (InOut exclusive access).
- BackendError still relevant (CPU/GPU mismatch).
- UseAfterFree still relevant (accessing released resources).

### Profiling
- Debug-only profiling. Zero overhead in release builds.

## Interview Round 9: Module Taxonomy & Batch Design

### Updated Module Taxonomy
| Module Type | Level | Description |
|---|---|---|
| Atomic | Module | Single .cpp shared library. Algorithm unit. |
| Pipeline | Module | Sequential chain. Variable list semantics. |
| Flow | Module | DAG, parallel where possible. |
| Batch | Module (wrapper) | Wraps a module, runs it in parallel over a collection. |
| Branch | System-level | NOT a module. Runtime routing handled by System. Future design. |

All Modules are STATIC (structure known at build time). Branch is dynamic/System-level.

### Batch Module Design
- **Transparent**: Inner module doesn't know it's batched. Sees a single element.
- **Split**: Programmable. Predefined strategies + custom modules.
  - `batch_tensor_dim`: Split tensor along a dimension (e.g., frame dimension)
  - `batch_sequence`: Split a sequence (List<T>) into individual elements
  - Custom: User provides a split module
- **Gather**: Programmable. Mirrors split strategy.
- **Race detection**: ERROR on overlapping InOut only. Gather module handles output assembly. Build-time detection impossible due to variable data sizes.
- **Parallelism**: Scheduler handles distribution of batch elements across threads.

### Batch YAML Structure
```yaml
module:
  name: batch_example
  type: batch
  inputs: [...]
  outputs: [...]
  split: { strategy + config }
  gather: { strategy + config }
  inner: { module reference }
  mapping: { split elements -> inner inputs, inner outputs -> gather }
```

## Interview Round 10: Validation Layer Redesign

### Validation Errors (New Model)
| Error | Phase | Description |
|---|---|---|
| BindingConflict | Build | Same name as both Input and Output in one module |
| UnresolvedBinding | Build | Module inputs "X" but no upstream produces it |
| TypeMismatch | Build | Input expects Tensor, upstream produces Scalar |
| UnusedOutput | Build (warning) | Output that no downstream module consumes |
| CyclicDependency | Build | Module graph contains a cycle |
| InOutRace | Schedule | Two parallel modules both InOut same data |
| InOutReadRace | Schedule | InOut + Input on same data in same parallel group |
| BatchOverlap | Schedule | Batch split produces overlapping InOut regions |
| InOutNoWrite | Runtime (debug) | InOut declared but never written — should be Input |
| UseAfterFree | Runtime | Accessing released managed pool resource |
| BackendMismatch | Runtime (future) | CPU module accesses GPU data |

### Validation Phases
1. **Build-time**: YAML parsing + module loading. Checks binding declarations, type compatibility, graph structure.
2. **Schedule-time**: DAG analysis. Checks parallelism safety, race conditions.
3. **Runtime** (debug only): Execution-time checks. InOut write detection, resource lifetime.

## Interview Round 11: Book Structure

### Two Books
1. **Tutorial (docs/guide/)**: Researcher-facing. "How to use GEF." Hands-on, example-driven, progressive.
2. **Architecture Reference (docs/design/)**: Engineer-facing. "How GEF works." Hub + deep-dive files, design rationale.

### Old docs: REPLACED by the new books. Delete existing 6 files.

### Writing Style: Rust Book style — technical but accessible, code examples, clear explanations, progressive difficulty.
### Language: English.

### Tutorial TOC (docs/guide/)
00-introduction.md, 01-quick-start.md, 02-your-first-module.md, 03-bindings.md, 04-composing-pipelines.md, 05-flows-and-parallelism.md, 06-batch-processing.md, 07-resource-management.md, 08-debugging.md, 09-external-libraries.md, 10-example-projects.md

### Architecture Reference (docs/design/)
overview.md (hub), system.md, module-system.md, binding-model.md, resource-pool.md, scheduler.md, validation-layer.md, yaml-config.md, plugin-architecture.md, external-adapters.md, error-handling.md, development-roadmap.md

### Code Examples: Compilable C++ (real, working examples once GEF is built).
### Diagrams: ASCII art only (no Mermaid).
### Philosophy: Dedicated section in introduction + rationale in each chapter.

## Scope Boundaries
- INCLUDE: Complete design documentation as a book/tutorial
- EXCLUDE: Any actual code implementation
