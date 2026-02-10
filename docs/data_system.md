# Data System — Bindings and Scopes

## Overview
This document defines the data model used by `Module`s: how values are bound, how their lifetimes are scoped, and where they are visible during execution. Bindings come in three kinds — `Config`, `Variable`, and `State` — each with distinct mutability and scope rules.

## Binding Kinds

| Kind       | Mutability | Scope                               | Typical use                     |
|------------|------------|-------------------------------------|---------------------------------|
| `Config`   | Immutable  | `Module` scope                      | Constants, algorithm parameters |
| `Variable` | Mutable    | Declared set in enclosing `Module`  | Data flow across `Module`s      |
| `State`    | Mutable    | `Module` scope                      | Shared, evolving state          |


## Scope Model
- `Module` scope
  - The time window while a specific enclosing `Module` is executing.
  - `Config` and `State` are accessible throughout this window within that execution.
- Variable scope (declared visibility set)
  - `Variable`s exist only where explicitly declared: in the `Module` that outputs them and in each `Module` that declares them as inputs within the same execution.
  - Input `Variable`s provided to an enclosing `Module` are available within that execution and visible only to `Module`s that explicitly declare them as inputs.

## Concurrency and Parallelism
- Core idea: the engine may execute `Module`s concurrently (multi‑threaded), and the granularity is determined by `Variable` relationships.
- Within an executing `Module` composed of sub‑`Module`s, readiness and parallelism derive from declared `Variable` inputs/outputs:
  - A `Module` becomes ready when all required input `Variable`s are available in the current execution.
  - Two `Module`s can run in parallel when their required inputs are available and neither depends on the other’s outputs.
  - When a `Module` requires another’s output `Variable`, the writer must complete before the reader starts.
- In practice this induces a DAG over `Variable` dependencies, from which the scheduler selects any ready set of `Module`s to run in parallel.

### State safety (race avoidance)
- `State` is mutable. Parallel execution must prevent races around the same `State`.
- Rules enforced by validation and scheduling:
  - Write‑exclusive: no parallel group may contain two `Module`s that both write to the same `State`.
  - Read‑while‑write: no parallel group may contain a `Module` writing a `State` and another reading that same `State`.
  - Read‑multiple: multiple reads of the same `State` in parallel are allowed when there is no write in the group.
- If a potential race on `State` is detected (e.g., two ready sub‑`Module`s both write the same `State`), the engine raises an error and reports the conflicting `Module`s and `State` so the user can introduce explicit ordering or refactor.

### Immutability preference
- The same underlying data may be bound as `State` (mutable) or as `Config` (immutable) depending on intended use.
- Prefer `Config` whenever mutation is not required; this improves safety and increases opportunities for parallelism.
- When a value is bound as `State` but a `Module` does not modify it during execution, the engine should emit a warning suggesting to use `Config` instead for that binding.

 
 
 
