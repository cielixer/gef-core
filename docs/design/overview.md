# GEF Architecture Overview

This document is the entry point for the **Architecture Reference** — a collection
of deep-dive documents that explain *how GEF works* and *why it was designed this
way*.  If you are a researcher who wants to *use* GEF, start with the
[Tutorial](../guide/00-introduction.md) instead.

---

## At a Glance

GEF (General Engine Framework) is a modular runtime for building engines and
data-processing programs.  Researchers write small, self-contained **Modules**;
engineers wire them together into **Pipelines**, **Flows**, and **Batch**
processors; and the **System** executes the whole graph with automatic
parallelism and resource management.

```
                         ┌──────────────────────────────────────┐
                         │              System                  │
                         │  (top-level program / final product) │
                         └──────────┬───────────────────────────┘
                                    │ owns
                    ┌───────────────┼───────────────┐
                    │               │               │
              ┌─────▼─────┐  ┌─────▼─────┐  ┌──────▼──────┐
              │ Scheduler  │  │ Resource  │  │   Module    │
              │            │  │   Pool    │  │   Graph     │
              │ Static     │  │           │  │             │
              │ + Dynamic  │  │ Managed   │  │ Atomic      │
              │            │  │ Unmanaged │  │ Pipeline    │
              └─────┬──────┘  └─────┬─────┘  │ Flow       │
                    │               │        │ Batch       │
                    │               │        └──────┬──────┘
                    │               │               │
                    └───────────────┼───────────────┘
                                    │ uses
                         ┌──────────▼──────────────┐
                         │    Binding Model         │
                         │  Input / Output / InOut  │
                         │  + Config                │
                         └──────────┬──────────────┘
                                    │ validated by
                         ┌──────────▼──────────────┐
                         │   Validation Layer       │
                         │  Build / Schedule /      │
                         │  Runtime checks          │
                         └─────────────────────────┘
```

---

## Core Concepts

| Concept | One-liner | Deep Dive |
|---------|-----------|-----------|
| **System** | Top-level program that owns everything | [system.md](system.md) |
| **Module** | Unit of computation (Atomic, Pipeline, Flow, Batch) | [module-system.md](module-system.md) |
| **Binding** | Typed data contract: Input, Output, InOut, Config | [binding-model.md](binding-model.md) |
| **ResourcePool** | Managed (refcounted) or Unmanaged (System-controlled) memory | [resource-pool.md](resource-pool.md) |
| **Scheduler** | Static DAG analysis + Dynamic runtime decisions | [scheduler.md](scheduler.md) |
| **Validation** | Three-phase checking: Build → Schedule → Runtime | [validation-layer.md](validation-layer.md) |
| **YAML Config** | Declarative module composition and wiring | [yaml-config.md](yaml-config.md) |
| **Plugin** | Shared-library loading, hot-reload, metadata | [plugin-architecture.md](plugin-architecture.md) |
| **Adapters** | Integration with external libraries (Eigen, etc.) | [external-adapters.md](external-adapters.md) |
| **Errors** | Hybrid error model: exceptions + Result types | [error-handling.md](error-handling.md) |
| **Roadmap** | Phased development plan | [development-roadmap.md](development-roadmap.md) |

---

## Data Flow

Data moves through the system along edges determined by **bindings**:

```
  ┌─────────┐  Output "blurred"   ┌────────────┐  Output "edges"   ┌────────┐
  │  Blur   ├────────────────────►│  Threshold  ├──────────────────►│  Save  │
  │ (Atomic)│                     │  (Atomic)   │                   │(Atomic)│
  └────┬────┘                     └──────┬──────┘                   └────┬───┘
       │                                 │                                │
  Input "image"                   Input "blurred"                  Input "edges"
       │                                 │                                │
       ▼                                 ▼                                ▼
  ┌──────────────────────────────────────────────────────────────────────────┐
  │                           ResourcePool                                  │
  │   Managed: transient intermediates (auto refcount)                      │
  │   Unmanaged: persistent data (System-controlled lifetime)               │
  └──────────────────────────────────────────────────────────────────────────┘
```

**Rules:**
- **Input** — read-only; multiple modules may read concurrently.
- **Output** — produces new data; no conflicts.
- **InOut** — in-place mutation; exclusive access, serialised by the scheduler.
- **Config** — immutable, engine-lifetime settings.

Same name declared as both Input and Output within one module is a **build-time
error** — this prevents tracking ambiguity entirely.

---

## Module Taxonomy

```
                        ┌──────────┐
                        │  Module  │
                        └────┬─────┘
               ┌─────────┬──┴──┬──────────┐
               │         │     │          │
          ┌────▼───┐ ┌───▼──┐ ┌▼────┐ ┌───▼──┐
          │Atomic  │ │Pipe- │ │Flow │ │Batch │
          │        │ │line  │ │     │ │      │
          │ .cpp   │ │seq.  │ │DAG  │ │par.  │
          │ .so    │ │chain │ │     │ │over  │
          └────────┘ └──────┘ └─────┘ │coll. │
                                      └──────┘
```

| Type | Structure | Execution | Defined In |
|------|-----------|-----------|------------|
| **Atomic** | Single .cpp → shared library | One `execute(Context&)` call | C++ file |
| **Pipeline** | Ordered list of modules | Sequential, skip-connections OK | YAML |
| **Flow** | DAG of modules | Parallel where bindings allow | YAML |
| **Batch** | Wrapper around any module | Runs inner module N times in parallel | YAML |

All modules are **static** — their structure is known at build time.  Branch
(conditional routing) is a **System-level** concern, not a module type.

---

## Scheduling

The scheduler operates at **two levels**:

1. **Static Scheduler** — Analyses binding declarations, builds a DAG, and
   pre-computes parallelisable groups before execution begins.
2. **Dynamic Scheduler** — Sits on top of the static scheduler and handles
   runtime decisions (batch distribution, future branching).  It invokes the
   static scheduler for sub-graphs as needed.

Engineers choose (or implement) a scheduler strategy via System configuration.

See [scheduler.md](scheduler.md) for the full design.

---

## Validation Phases

GEF validates the module graph at **three** increasingly concrete phases:

| Phase | When | Example Checks |
|-------|------|----------------|
| **Build** | YAML parse + module load | BindingConflict, UnresolvedBinding, TypeMismatch, CyclicDependency |
| **Schedule** | DAG analysis | InOutRace, InOutReadRace, BatchOverlap |
| **Runtime** | Execution (debug only) | InOutNoWrite, UseAfterFree, BackendMismatch (future) |

Build-time catches the most common mistakes.  Schedule-time catches parallelism
hazards.  Runtime checks are **debug-only** with **zero overhead** in release
builds.

See [validation-layer.md](validation-layer.md) for the complete error catalogue.

---

## Glossary

| Term | Definition |
|------|------------|
| **Atomic** | Smallest module — one .cpp file compiled to a shared library |
| **Batch** | Wrapper that runs an inner module in parallel over a collection |
| **Binding** | Typed contract declaring how a module reads/writes data |
| **Config** | Immutable, engine-lifetime setting injected into a module |
| **Context** | Object passed to `execute()`; provides `input<T>()`, `output<T>()`, `inout<T>()`, `config<T>()` |
| **Flow** | DAG of modules with automatic parallelism |
| **GEF_STAGE** | Macro that declares an Atomic module |
| **InOut** | Binding for in-place mutation with exclusive access |
| **Input** | Read-only binding; safe for concurrent access |
| **Managed pool** | Reference-counted ResourcePool for transient data |
| **Module** | Generic term for any computation unit (Atomic, Pipeline, Flow, Batch) |
| **Output** | Binding that produces new data |
| **Pipeline** | Sequential chain of modules with implicit name-matching |
| **ResourcePool** | Memory manager — Managed (refcounted) or Unmanaged (manual) |
| **Scheduler** | Component that determines execution order and parallelism |
| **System** | Top-level program; owns scheduler, pools, and the module graph |
| **Unmanaged pool** | System-controlled ResourcePool for persistent data |
| **Validation** | Three-phase correctness checking (Build → Schedule → Runtime) |
| **Wiring** | Explicit edge definitions in YAML when implicit matching is insufficient |

---

## Design Principles

1. **Researcher-first DX** — Modify one .cpp file, rebuild one shared library,
   run immediately.  No full-project rebuilds.
2. **Explicit over implicit** — Every binding declares intent (read / produce /
   mutate).  Ambiguity is a build error.
3. **Parallelism from declarations** — The scheduler extracts concurrency
   automatically from Input / Output / InOut annotations.
4. **Composition over inheritance** — Modules are composed via YAML wiring, not
   class hierarchies.
5. **Safety with zero overhead** — Debug-only checks (InOut write detection,
   use-after-free) vanish in release builds.
6. **Memory efficiency** — InOut supports in-place mutation of large tensors;
   Managed / Unmanaged pools balance convenience and control.

---

## Where to Go Next

**If you want to…**

- Understand the top-level runtime → [System](system.md)
- Learn how modules compose → [Module System](module-system.md)
- Dive into data contracts → [Binding Model](binding-model.md)
- See YAML examples → [YAML Config](yaml-config.md)
- Learn about memory management → [Resource Pool](resource-pool.md)
- Understand execution ordering → [Scheduler](scheduler.md)
- See what errors GEF catches → [Validation Layer](validation-layer.md)
- Learn about hot-reload → [Plugin Architecture](plugin-architecture.md)
- Integrate external libraries → [External Adapters](external-adapters.md)
- Understand error philosophy → [Error Handling](error-handling.md)
- See the development plan → [Development Roadmap](development-roadmap.md)

Or start from the researcher side: [Tutorial — Introduction](../guide/00-introduction.md).
