# Scheduler Architecture

The GEF scheduler is the primary engine of execution, responsible for orchestrating the lifecycle of modules and managing the distribution of computational work across available hardware resources. It transforms a high-level dataflow graph into a concrete execution plan, ensuring that all data dependencies are respected while maximizing concurrent execution.

In a researcher-first framework like GEF, the scheduler must be both invisible and powerful. Researchers should focus on writing their algorithms (Atomic modules) and composing them (Flows and Pipelines), while the scheduler handles the complexities of multi-threading, synchronization, and resource management.

## Overview: Two-Level Architecture

GEF adopts a two-level scheduling architecture to decouple the structural analysis of the module graph from the runtime management of data and control flow. This separation allows GEF to remain highly efficient (static analysis overhead is paid only once) while retaining the flexibility to handle dynamic runtime scenarios.

1.  **Static Scheduler (Phase 1)**: Analyzes the immutable structure of Flow and Pipeline modules. It identifies the "skeleton" of the execution—which modules depend on which data, and which modules can safely run in parallel. This analysis happens at initialization or whenever a composition is reloaded.
2.  **Dynamic Scheduler (Phase 2)**: Executes the schedules produced by the static layer. It handles the "meat" of the execution—routing data, managing the thread pool, and resolving runtime-variable structures like Batch element counts or future Branch paths.

## Static Scheduler

The Static Scheduler is responsible for all analysis that can be performed without knowing the actual values of the data being processed.

### Binding-Based Dependency Analysis
Every Atomic module in GEF explicitly declares its data requirements using `Input`, `Output`, and `InOut` bindings. The Static Scheduler parses these declarations across all modules within a Flow or Pipeline to establish a data lineage.
- If Module A produces an `Output` named "raw_image" and Module B requires an `Input` named "raw_image", the scheduler records a direct dependency (A -> B).
- If Module C uses `InOut` on "raw_image", it creates a synchronization point that serializes it against any other module reading or writing "raw_image".

### Directed Acyclic Graph (DAG) Generation
Once all bindings are analyzed, the scheduler constructs a DAG. Nodes in this graph represent module executions, and directed edges represent the flow of data. The GEF validation layer immediately checks this graph for cyclic dependencies, which are treated as fatal build-time errors.

### Parallel Group Identification
The core task of the Static Scheduler is to identify "parallel groups." A parallel group is a set of modules that share no conflicting data dependencies and whose prerequisite data is already available. For example, if two different filters both read from the same `Input` image, they are placed in the same parallel group and can be executed concurrently on separate threads.

### Pre-computation and Caching
To achieve "researcher-first" performance, the Static Scheduler pre-computes the entire topological order and parallel grouping. This resulting "Execution Plan" is cached. At runtime, the engine simply follows this plan, eliminating the need to re-scan dependencies or re-sort the graph during the hot-path of execution.

## Dynamic Scheduler

The Dynamic Scheduler is the runtime component that breathes life into the static Execution Plan.

### Execution Orchestration
The Dynamic Scheduler iterates through the pre-computed parallel groups. It dispatches modules within a group to the System's thread pool and waits for their completion before proceeding to the next group. This ensures that the "happens-before" relationships defined by the DAG are strictly enforced.

### Sub-graph Invocation
GEF supports nested compositions (e.g., a Flow inside another Flow). When the Dynamic Scheduler encounters a composite module, it recursively invokes the scheduling logic for that sub-graph. It treats the entire sub-graph as a single unit within the parent schedule, maintaining a clean hierarchy of execution.

### Runtime Branching and Routing
In future versions of GEF, the Dynamic Scheduler will be the home for Branch modules. Since a Branch determines which path to take based on runtime data (e.g., "if the image is too dark, run the enhancement pipeline"), it cannot be resolved by the Static Scheduler. The Dynamic Scheduler will evaluate the branch condition and activate the appropriate static sub-schedule on the fly.

## Parallelism Rules by Binding Type

The ability to run modules in parallel is derived purely from the semantics of their bindings. This creates a "safety-by-design" environment where race conditions are caught before execution begins.

| Binding | Access Pattern | Parallelism Rule | Detailed Behavior |
| :--- | :--- | :--- | :--- |
| **Input** | Read-only | **Safe** | Multiple modules can read the same Input concurrently without any synchronization overhead. |
| **Output** | Write (new) | **Safe** | Since an Output always creates a new resource in the ResourcePool, it cannot conflict with existing data. |
| **InOut** | Read+Write (exclusive) | **Serializes** | An InOut binding requires exclusive access to the resource. It cannot run concurrently with any other reader (Input) or writer (InOut) of the same data. |
| **Config** | Read-only (immutable) | **Always safe** | Config data is injected by the System and is immutable for the duration of the engine's lifetime. All modules can access all Configs simultaneously. |

## Scheduler as Abstract Interface

GEF does not mandate a single scheduling algorithm. Instead, it provides a `IScheduler` abstract interface. This allows the framework to be adapted to vastly different hardware and performance profiles.

### Engineer Configurability
Engineers (the persona responsible for building the final System) can configure the scheduler via the System settings. Common configuration points include:
- **Thread Pool Scaling**: Defining how many worker threads are available for parallel modules.
- **Priority Mapping**: Assigning higher execution priority to specific Flows or Pipelines.
- **Affinity Settings**: Binding specific modules to specific CPU cores or NUMA nodes for performance.

### Custom Implementations
While GEF provides a robust default scheduler capable of handling most research and production workloads, developers can implement their own `IScheduler`. This is useful for:
- **Hard Real-Time Systems**: Where execution timing must be strictly deterministic and jitter-free.
- **Distributed Execution**: Where the "modules" might actually be running on different physical machines.
- **Specialized Hardware**: Schedulers optimized for specific NPU or FPGA architectures.

## Deterministic Tie-Breaking

Parallelism can often lead to non-deterministic execution orders if not handled carefully. GEF enforces strict determinism in its scheduling to ensure that bugs are reproducible and results are consistent across runs.

When the Static Scheduler identifies multiple modules that *could* run in parallel, it assigns them a fixed execution priority based on:
1.  **Topological Rank**: Modules earlier in the dependency chain are prioritized.
2.  **Lexicographical Sorting**: If two modules are at the same rank, they are sorted alphabetically by their unique name in the YAML configuration.

This tie-breaking ensures that even on a single-threaded scheduler, the execution order is identical every time. This is particularly valuable for researchers who need to debug complex data transformations where timing-related bugs could otherwise be impossible to catch.

## Batch Element Distribution

The Batch module is a special case that requires tight integration between the Dynamic Scheduler and the ResourcePool.

### Data-Parallel Execution
A Batch module wraps an inner module and runs it over a collection (like a List or a Tensor dimension). The Dynamic Scheduler determines the size of the collection at runtime and partitions it into individual elements.

### Load Balancing and Granularity
The scheduler distributes these elements across the worker threads. If a batch contains 1000 items and the system has 8 cores, the scheduler manages the queueing and execution of these 1000 tasks, ensuring that all cores remain busy.
- **Work Stealing**: The default scheduler uses a work-stealing thread pool to ensure that load is balanced dynamically if some batch elements take longer to process than others.
- **Task Chunking**: To minimize scheduling overhead, the scheduler can group small batch elements into "chunks," reducing the total number of tasks submitted to the pool.

### Gather Synchronization
Once all batch elements have been processed, the Dynamic Scheduler executes the "Gather" phase. This acts as a synchronization barrier—no modules downstream of the Batch module can start until every single element in the batch has been successfully processed and recombined.

## Resource Safety and Lifetimes

The scheduler plays a vital role in ensuring the safety of the GEF `ResourcePool`. By understanding the execution schedule, it can assist in managing resource lifetimes.

- **Borrow Management**: The scheduler ensures that resources are "borrowed" from the pool just before a module starts and "returned" immediately after it finishes.
- **Managed Pool Integration**: For resources in a Managed ResourcePool, the scheduler's knowledge of the DAG allows the system to predict when a resource's refcount will reach zero, enabling proactive memory reclamation.
- **Race Prevention**: The scheduler's enforcement of InOut serialization is the primary defense against data races in the memory-efficient in-place mutation model.

## Relationship to Validation

The scheduler is the primary consumer of the GEF Validation Layer during the "Schedule-time" phase. Once a schedule is generated, the validation layer performs the following checks:

- **InOutRace**: Verifies that no two modules in a parallel group are attempting to modify the same resource using `InOut`.
- **InOutReadRace**: Verifies that no module is attempting to modify a resource while another module in the same group is reading it as an `Input`.
- **BatchOverlap**: For Batch modules with custom split strategies, it checks (where possible) that the split regions do not overlap, which would lead to race conditions during parallel `InOut` operations.

## Future Work: Dynamic Branching

Current scheduling is optimized for "Flows"—static DAGs. Future work involves extending the Dynamic Scheduler to support System-level branching. This will allow for complex control flow (loops, conditional exits, and dynamic routing) while maintaining the performance and safety guarantees of the GEF module system. 

The goal is to move control logic out of the C++ modules and into the System-level configuration. By doing so, GEF allows researchers to experiment with high-level logic (e.g., "retry this detection Flow if the confidence is too low") without recompiling their algorithmic units. This architectural choice reinforces the "researcher-first" philosophy by keeping the development loop as fast and flexible as possible.

Ultimately, the GEF scheduler aims to provide a predictable, high-performance foundation that allows complex scientific and engineering workflows to scale from a single developer's workstation to high-performance computing clusters without changing a single line of algorithmic code.



