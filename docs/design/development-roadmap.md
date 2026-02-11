# GEF Development Roadmap

## Overview
The General Engine Framework (GEF) follows an incremental development approach. Each phase is designed to deliver a functional subset of the engine, allowing for early testing and validation of core concepts. By prioritizing the researcher-first developer experience (DX), GEF evolves from a basic resource management layer into a sophisticated parallel execution engine.

This roadmap outlines the logical progression of GEF's architecture. It is strictly ordered by dependency; each phase provides the necessary foundation for the next, ensuring that no feature depends on future, unimplemented designs.

## Build System
GEF is built using a modern C++ stack optimized for performance, modularity, and rapid development cycles:
- **Language Standard**: C++23, leveraging advanced features for compile-time safety and expressive APIs.
- **Package Management**: `pixi`, ensuring reproducible environments and seamless dependency resolution.
- **Build Tool**: `CMake`, providing a standard and flexible build configuration across platforms.

## Development Phases

### Phase 1: Core ResourcePool
The foundation of GEF is its data management layer. Phase 1 focuses on the CPU-based Managed ResourcePool.
- Implementation of the `ResourcePool` abstraction.
- Introduction of reference-counted resource management (Managed Pool).
- Basic borrow rules to ensure memory safety during concurrent access.
- Support for fundamental data types (Tensors, Scalars).

### Phase 2: Module System
With resource management in place, the framework introduces the plugin-based module architecture.
- Development of the Atomic module loading system.
- Creation of the macro framework (`GEF_MODULE`) for researcher-friendly module declaration.
- Implementation of the `Context` API for module-engine interaction.
- Support for dynamic loading of shared libraries (.so/.dylib).

### Phase 3: Binding System
This phase defines how data enters and exits modules, establishing the core dataflow semantics.
- Implementation of `Input`, `Output`, `InOut`, and `Config` binding kinds.
- Build-time validation for binding conflicts (e.g., same name as Input and Output).
- Type checking between connected modules.
- Explicit mutation support via `InOut` bindings.

### Phase 4: Pipeline Composition
Building upon Atomic modules, Phase 4 enables sequential composition.
- Development of the YAML parser for module configuration.
- Implementation of the `Pipeline` module type.
- Support for implicit wiring (matching by name) and skip-connections.
- Sequential execution engine for linear chains of modules.

### Phase 5: Flow Composition
Transitioning from linear sequences to complex graphs.
- Implementation of the `Flow` module type for DAG (Directed Acyclic Graph) structures.
- Development of the Static Scheduler to analyze dependencies and identify parallelizable groups.
- Multi-threaded execution of independent modules.
- Support for explicit wiring syntax in YAML.

### Phase 6: Batch Module
Enabling data-parallelism at the module level.
- Creation of the `Batch` wrapper module.
- Implementation of split/gather strategies (e.g., `batch_tensor_dim`, `batch_sequence`).
- Parallel map execution across batch elements.
- Transparent inner module execution (the inner module remains unaware of batching).

### Phase 7: External Adapters
Integrating GEF with the broader research ecosystem.
- Development of the adapter pattern for external library types.
- Initial support for Eigen (matrix and linear algebra).
- Zero-copy views where possible between GEF and external library buffers.

### Phase 8: Unmanaged ResourcePool and System API
Finalizing the top-level control structures for production-grade applications.
- Implementation of the Unmanaged ResourcePool for persistent, System-controlled data.
- Development of the `System` API to manage the execution lifecycle and resource strategies.
- Support for long-lived resources (e.g., model weights, lookup tables).

### Phase 9: Debug Tooling
Enhancing the developer experience with advanced introspection and validation.
- Implementation of the debug-only profiling system.
- Runtime write detection for `InOut` bindings to identify missing mutations.
- Enhanced error reporting for borrow violations and lifetime issues.
- Performance visualization tools for the Static Scheduler.

## Future Work
Beyond the core roadmap, several aspirational features are planned to expand GEF's capabilities:
- **GPU Backends**: Native support for CUDA and Vulkan compute for high-performance tensor operations.
- **Branch Routing**: System-level runtime path selection (if/else) for dynamic execution graphs.
- **GUI Composition Tool**: A visual module graph editor for designing Flows and Pipelines without manual YAML editing.
- **Custom Configuration DSL**: A specialized language for expressing module graphs with more conciseness than YAML.
- **Additional Adapters**: Native integration for OpenCV (computer vision) and ArrayFire (heterogeneous computing).

## Principles
1. **Incremental Delivery**: Each phase builds directly upon the code and concepts established in previous phases.
2. **Independence**: No phase relies on the design or implementation of a future phase.
3. **Functionality**: Completion of any phase results in a usable and testable subset of the framework.
4. **Safety First**: Memory safety and race condition prevention are baked into the core abstractions (ResourcePool, Bindings) from Phase 1.
5. **Zero Overhead**: Advanced validation and profiling are targeted for debug builds, ensuring production performance is never compromised.
