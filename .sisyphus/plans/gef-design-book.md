# GEF Design Book — Documentation Plan

## TL;DR

> **Quick Summary**: Write two comprehensive documentation books for GEF (General Engine Framework) — a Tutorial for researchers and an Architecture Reference for engineers — replacing the existing 6 design docs. All content is based on the extensively interviewed design (11 rounds). No code implementation.
> 
> **Deliverables**:
> - 11 Tutorial chapters in `docs/guide/`
> - 12 Architecture Reference files in `docs/design/`
> - Deletion of 6 obsolete design docs from `docs/`
> - Updated `README.md` with project overview and links to docs
> 
> **Estimated Effort**: Large
> **Parallel Execution**: YES — 3 waves
> **Critical Path**: Task 1 (Foundation) → Tutorial Tasks → Architecture Tasks → Task 25 (Cleanup)

---

## Context

### Original Request
User wants to consolidate all GEF design knowledge from their brain into documentation. The framework has been designed through 11 interview rounds covering modules, bindings, YAML config, System architecture, ResourcePool, scheduling, validation, and batch processing. The existing 6 design docs are outdated (use superseded terminology and binding model). The deliverable is two "books" — a researcher-facing tutorial and an engineer-facing architecture reference — written in Rust Book style with compilable C++ examples and ASCII art diagrams.

### Interview Summary
**Key Discussions**:
- **Module System**: 4 types (Atomic, Pipeline, Flow, Batch) + Branch at System level. All composable. "Stage" terminology removed — everything is "Module."
- **Binding Model (REDESIGNED from scratch)**: Input (read-only), Output (new data), InOut (in-place mutation), Config (immutable engine settings). Old State/View/Variable bindings REMOVED.
- **Stage API**: Macros (GEF_MODULE, GEF_INPUT, GEF_OUTPUT, GEF_INOUT, GEF_CONFIG) + execute(Context&). Proxy return types for auto safety.
- **YAML Config**: Unified format for Pipeline/Flow. Mandatory `inputs`/`outputs`/`configs` interface. `modules:` field. Implicit wiring by name + explicit override. Arrow syntax. Multi-file includes.
- **System**: Top-level orchestrator = the program itself. Owns ResourcePools, runs scheduler. Researcher writes modules, Engineer builds System.
- **ResourcePool**: Two types — Managed (refcounted) and Unmanaged (System-controlled persistent). Single ownership. Old 3-scope model replaced.
- **Scheduler**: Two levels — Static (DAG analysis) + Dynamic (runtime routing). Engineer configures.
- **Batch**: Module wrapper running inner module in parallel over collection. Transparent to inner. Programmable split/gather.
- **Validation**: 3 phases (Build/Schedule/Runtime). 11 error types. Debug-only profiling.
- **Plugin Architecture**: Each atomic module = .cpp → .so/.dylib. Directory scan. Query function metadata.
- **Error Handling**: Hybrid (exceptions + Result types).

### Metis Review
**Identified Gaps** (addressed):
- Documentation voice (spec vs existing system) → Resolved: Present-tense with a status disclaimer acknowledging design phase
- Running example domain needed → Resolved: Image processing pipeline threads through tutorial
- Cross-referencing strategy between books → Resolved: Tutorial links to Architecture Reference for deep dives; Reference is self-contained per file
- Architecture Reference reading order → Resolved: overview.md links in recommended order; each file is readable independently
- README.md update needed → Added to plan
- Table of contents for each book → Added to plan (each book gets an index)
- Multi-file include path semantics → Resolved: Relative to including file (documented in yaml-config.md)
- Batch empty collection handling → Documented as zero iterations (silent success) with optional warning
- Config key collision → Documented: System-level disambiguation via qualified names

---

## Work Objectives

### Core Objective
Replace the 6 outdated design docs with two comprehensive, well-structured documentation books that capture the complete GEF design as established through 11 interview rounds.

### Concrete Deliverables
- `docs/guide/` — 11 markdown files (Tutorial book)
- `docs/design/` — 12 markdown files (Architecture Reference)
- Updated `README.md`
- Deletion of 6 old docs

### Definition of Done
- [ ] All 23 new markdown files exist with substantive content (≥80 lines each)
- [ ] All 6 old docs deleted
- [ ] Zero instances of banned terminology in new docs
- [ ] Zero Mermaid diagrams in new docs
- [ ] All Tutorial chapters (02-09) contain C++ code blocks
- [ ] YAML config chapters contain YAML code blocks
- [ ] Architecture Reference overview.md links to all 11 other design files
- [ ] README.md updated with project description and doc links

### Must Have
- Rust Book writing style (technical, accessible, progressive)
- Compilable C++ examples (spec-compliant, using GEF macros and Context API as designed)
- ASCII art diagrams for visual concepts
- Dedicated philosophy section in introduction
- Consistent terminology throughout (Input/Output/InOut/Config, Module, Flow, Atomic)
- Image processing as the running example domain in the Tutorial
- English language throughout

### Must NOT Have (Guardrails)
- **Superseded terminology**: State (as binding), View (as binding), Variable (as binding), Workflow (use Flow), Consistent scope, promote()
- **Mermaid diagrams**: All diagrams must be ASCII art
- **Invented features**: No GPU APIs, networking, GUI tools, custom DSL (these are future only)
- **Internal implementation details**: No scheduler algorithms, ResourcePool allocator internals, Result<T> type design
- **More than 3 example projects**: Chapter 10 is ≤3 examples, ≤2 pages each
- **Adapter designs beyond Eigen**: External adapters chapter covers the pattern + Eigen only
- **"stages:" in YAML**: Always use "modules:"
- **Inheritance-based Stage API**: The macro + execute(Context&) pattern only
- **Human-dependent acceptance criteria**: All verification is automated

---

## Verification Strategy

> **UNIVERSAL RULE: ZERO HUMAN INTERVENTION**
> All verification via automated checks.

### Test Decision
- **Infrastructure exists**: N/A (documentation task)
- **Automated tests**: N/A
- **Framework**: N/A

### Agent-Executed QA Scenarios (MANDATORY — ALL tasks)

Every task includes verification via shell commands checking file existence, content, and terminology compliance.

**Verification Tool by Deliverable Type:**

| Type | Tool | How Agent Verifies |
|------|------|-------------------|
| **Markdown files** | Bash (grep, wc, ls) | Check existence, line count, content presence, banned terms |
| **Cross-references** | Bash (grep) | Verify links between files |
| **Terminology compliance** | Bash (grep) | Zero matches for banned terms |

---

## Execution Strategy

### Parallel Execution Waves

```
Wave 1 (Start Immediately):
+-- Task 1: Create directory structure + foundation reference
+-- Task 2: Tutorial 00-introduction.md
+-- Task 13: Architecture binding-model.md

Wave 2 (After Foundation):
+-- Tasks 3-12: Tutorial chapters 01-10 (sequential within)
+-- Tasks 14-23: Architecture Reference files (dependency-ordered within)

Wave 3 (After All Content):
+-- Task 24: Architecture overview.md (needs all other design files)
+-- Task 25: Delete old docs + update README
+-- Task 26: Final terminology and structural compliance check
```

### Dependency Matrix

| Task | Depends On | Blocks | Can Parallelize With |
|------|------------|--------|---------------------|
| 1 (Foundation) | None | All others | None |
| 2 (Intro) | 1 | 3 | 13 |
| 3-12 (Tutorial) | 1, sequential chain | 25 | 14-23 |
| 13-23 (Arch Ref) | 1, dependency-ordered | 24 | 3-12 |
| 24 (Overview hub) | 13-23 | 25 | None |
| 25 (Cleanup) | 3-12, 24 | 26 | None |
| 26 (Final check) | 25 | None | None |

### Agent Dispatch Summary

| Wave | Tasks | Recommended Agents |
|------|-------|-------------------|
| 1 | 1, 2, 13 | task(category="writing") for each |
| 2 | 3-12, 14-23 | Two parallel tracks of task(category="writing") |
| 3 | 24, 25, 26 | Sequential task(category="quick") |

---

## TODOs

> EVERY task writes markdown files only. No code implementation.
> The authoritative design source is `.sisyphus/drafts/gef-book-design.md`.

- [x] 1. Create directory structure and terminology reference

  **What to do**:
  - Create `docs/guide/` directory
  - Create `docs/design/` directory
  - Create an internal terminology reference at `.sisyphus/drafts/terminology-reference.md` containing:
    - Binding kinds: Input, Output, InOut, Config (with exact definitions)
    - Module types: Atomic, Pipeline, Flow, Batch (with exact definitions)
    - YAML field names: `modules:`, `inputs:`, `outputs:`, `configs:`, `wiring:`, `inner:`, `split:`, `gather:`
    - Macro names: GEF_MODULE, GEF_INPUT, GEF_OUTPUT, GEF_INOUT, GEF_CONFIG
    - Context API: ctx.input<T>(), ctx.output<T>(), ctx.inout<T>(), ctx.config<T>()
    - Banned terms: State (as binding), View (as binding), Variable (as binding), Workflow, Consistent scope, promote(), stages: (in YAML)
    - Running example domain: Image processing pipeline
  - This reference is used by ALL subsequent tasks for consistency

  **Must NOT do**:
  - Write any source code files
  - Create files outside docs/ and .sisyphus/

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Simple directory creation and reference file writing
  - **Skills**: []
    - No special skills needed

  **Parallelization**:
  - **Can Run In Parallel**: NO (foundation for all others)
  - **Parallel Group**: Wave 1 (starts immediately)
  - **Blocks**: All other tasks
  - **Blocked By**: None

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — All design decisions from 11 interview rounds. THE authoritative source.

  **Acceptance Criteria**:

  - [ ] `docs/guide/` directory exists
  - [ ] `docs/design/` directory exists
  - [ ] `.sisyphus/drafts/terminology-reference.md` exists with ≥50 lines

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Directories and reference file created
    Tool: Bash
    Steps:
      1. ls -d docs/guide docs/design
      2. Assert: both directories exist
      3. wc -l < .sisyphus/drafts/terminology-reference.md
      4. Assert: ≥50 lines
      5. grep -c "Input\|Output\|InOut\|Config" .sisyphus/drafts/terminology-reference.md
      6. Assert: ≥4 matches (all 4 binding kinds documented)
    Expected Result: Structure ready for content creation
  ```

  **Commit**: YES
  - Message: `docs: create directory structure and terminology reference for GEF books`
  - Files: `docs/guide/.gitkeep`, `docs/design/.gitkeep`, `.sisyphus/drafts/terminology-reference.md`

---

- [x] 2. Tutorial: 00-introduction.md — What is GEF?

  **What to do**:
  - Write `docs/guide/00-introduction.md`
  - Content:
    - What GEF is: General Engine Framework for modular computation pipelines
    - Philosophy: "explicit control + safe concurrency + single memory ownership"
    - Design status disclaimer: "GEF is currently in the design phase. Code examples show the intended API surface. The framework is not yet implemented."
    - Who this is for: Researchers (algorithm development) and Engineers (system building)
    - The two personas explained (Researcher vs Engineer)
    - What you'll learn in this tutorial (chapter overview / table of contents)
    - How to read this book (sequential, progressive)
    - Link to Architecture Reference for deep dives
  - Include ASCII art diagram showing the high-level architecture:
    ```
    System (the program)
      |
      +-- ResourcePool (Managed / Unmanaged)
      +-- Scheduler (Static + Dynamic)
      +-- Module Graph
            +-- Atomic Modules (.cpp -> .so)
            +-- Pipeline (sequential)
            +-- Flow (DAG, parallel)
            +-- Batch (data-parallel)
    ```
  - Dedicated philosophy section explaining:
    - Explicit over implicit
    - Safety without sacrificing performance
    - Single ownership of all resources
    - Researcher-friendly DX: modify one file, rebuild one .so, run immediately
    - Data-Oriented Design principles

  **Must NOT do**:
  - Use Mermaid diagrams
  - Use banned terminology
  - Include implementation details

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Prose-heavy introductory chapter requiring Rust Book style
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Task 1 completed first, then parallel with Task 13)
  - **Blocks**: Tasks 3-12
  - **Blocked By**: Task 1

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — Philosophy section, two-persona model, all design decisions
  - `.sisyphus/drafts/terminology-reference.md` — Terminology to use

  **Documentation References**:
  - Rust Book introduction style: https://doc.rust-lang.org/book/ch00-00-introduction.html — Reference for tone and structure

  **Acceptance Criteria**:

  - [ ] `docs/guide/00-introduction.md` exists with ≥120 lines
  - [ ] Contains "# " title heading
  - [ ] Contains "philosophy" or "Philosophy" section
  - [ ] Contains design status disclaimer
  - [ ] Contains ASCII art diagram (no Mermaid)
  - [ ] Zero banned terminology

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Introduction chapter is complete and compliant
    Tool: Bash
    Steps:
      1. test -f docs/guide/00-introduction.md
      2. wc -l < docs/guide/00-introduction.md → Assert ≥120
      3. head -1 docs/guide/00-introduction.md → Assert starts with "# "
      4. grep -ci "philosophy" docs/guide/00-introduction.md → Assert ≥1
      5. grep -c "design phase\|not yet implemented\|intended API" docs/guide/00-introduction.md → Assert ≥1
      6. grep -c "```mermaid" docs/guide/00-introduction.md → Assert 0
      7. grep -c "State binding\|View binding\|Variable binding\|Consistent scope\|promote()" docs/guide/00-introduction.md → Assert 0
    Expected Result: Complete introduction with philosophy, disclaimer, no banned terms
  ```

  **Commit**: YES (groups with Task 3)
  - Message: `docs: add tutorial introduction chapter`
  - Files: `docs/guide/00-introduction.md`

---

- [x] 3. Tutorial: 01-quick-start.md — Getting Started

  **What to do**:
  - Write `docs/guide/01-quick-start.md`
  - Content:
    - Prerequisites: C++23 compiler, pixi, CMake
    - Installing pixi and setting up the project
    - Project structure overview (where modules go, where YAML configs go, where .so files are built)
    - The "Hello World" of GEF: a minimal module that reads an input tensor and outputs a processed tensor
    - Build and run cycle: modify .cpp → build (only that .so rebuilds) → run
    - What just happened: brief explanation of the plugin loading and execution flow
  - Include compilable C++ example of the simplest possible module
  - Include example YAML config for a single-module pipeline
  - Include ASCII art showing the build/run cycle

  **Must NOT do**:
  - Deep dive into bindings (covered in Chapter 3)
  - Explain parallel execution (covered in Chapter 5)
  - Use banned terminology

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Tutorial writing with code examples
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (sequential within Tutorial track)
  - **Parallel Group**: Wave 2 (Tutorial track, sequential)
  - **Blocks**: Task 4
  - **Blocked By**: Tasks 1, 2

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — Plugin architecture, macro API, YAML format
  - `.sisyphus/drafts/terminology-reference.md` — Terminology
  - `pixi.toml` — Build system configuration reference

  **Acceptance Criteria**:

  - [ ] `docs/guide/01-quick-start.md` exists with ≥100 lines
  - [ ] Contains at least 1 C++ code block
  - [ ] Contains at least 1 YAML code block
  - [ ] Zero banned terminology

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Quick start chapter complete
    Tool: Bash
    Steps:
      1. test -f docs/guide/01-quick-start.md
      2. wc -l < docs/guide/01-quick-start.md → Assert ≥100
      3. grep -c '```cpp\|```c++' docs/guide/01-quick-start.md → Assert ≥1
      4. grep -c '```yaml' docs/guide/01-quick-start.md → Assert ≥1
      5. grep -c "State binding\|View binding\|Variable binding\|Workflow\|Consistent scope" docs/guide/01-quick-start.md → Assert 0
    Expected Result: Quick start with code and YAML examples
  ```

  **Commit**: YES (groups with Task 4)
  - Message: `docs: add tutorial quick-start chapter`
  - Files: `docs/guide/01-quick-start.md`

---

- [x] 4. Tutorial: 02-your-first-module.md — Anatomy of a Module

  **What to do**:
  - Write `docs/guide/02-your-first-module.md`
  - Content:
    - What is an Atomic Module (a .cpp file compiled as a shared library)
    - The macro declarations: GEF_MODULE, GEF_INPUT, GEF_OUTPUT, GEF_INOUT, GEF_CONFIG
    - The execute(Context&) function
    - Context API: ctx.input<T>(), ctx.output<T>(), ctx.inout<T>(), ctx.config<T>()
    - Proxy return types: why `auto` is safe (InputRef<T>, InOutRef<T>)
    - Behind the scenes: macros generate extern "C" metadata query function
    - Complete example: image processing module (blur) with full code
    - How the engine discovers and loads modules (directory scan)
  - Running example: Gaussian blur module

  **Must NOT do**:
  - Explain Pipeline/Flow composition (next chapters)
  - Deep dive into ResourcePool internals
  - Use inheritance-based patterns

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Core tutorial chapter, needs careful API explanation
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (sequential within Tutorial)
  - **Parallel Group**: Wave 2
  - **Blocks**: Task 5
  - **Blocked By**: Task 3

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — Stage API, macro design, proxy types, plugin metadata
  - `.sisyphus/drafts/terminology-reference.md` — Macro names, Context API methods

  **Acceptance Criteria**:

  - [ ] File exists with ≥150 lines
  - [ ] Contains ≥3 C++ code blocks (declarations, execute function, full example)
  - [ ] Contains GEF_MODULE, GEF_INPUT, GEF_OUTPUT macro usage
  - [ ] Contains ctx.input, ctx.output, ctx.inout, ctx.config usage
  - [ ] Explains proxy types (InputRef or similar)
  - [ ] Zero banned terminology

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: First module chapter is comprehensive
    Tool: Bash
    Steps:
      1. test -f docs/guide/02-your-first-module.md
      2. wc -l < docs/guide/02-your-first-module.md → Assert ≥150
      3. grep -c '```cpp\|```c++' docs/guide/02-your-first-module.md → Assert ≥3
      4. grep -c "GEF_MODULE\|GEF_INPUT\|GEF_OUTPUT" docs/guide/02-your-first-module.md → Assert ≥3
      5. grep -c "ctx\.input\|ctx\.output\|ctx\.inout\|ctx\.config" docs/guide/02-your-first-module.md → Assert ≥2
      6. grep -ci "proxy\|InputRef\|InOutRef" docs/guide/02-your-first-module.md → Assert ≥1
      7. grep -c "State binding\|View binding\|Variable binding" docs/guide/02-your-first-module.md → Assert 0
    Expected Result: Complete module anatomy with all API elements
  ```

  **Commit**: YES (groups with Task 5)
  - Message: `docs: add tutorial first-module and bindings chapters`
  - Files: `docs/guide/02-your-first-module.md`

---

- [x] 5. Tutorial: 03-bindings.md — Input, Output, InOut, Config

  **What to do**:
  - Write `docs/guide/03-bindings.md`
  - Content:
    - The 4 binding kinds with clear definitions and comparison table
    - Input: read-only, concurrent-safe, const reference semantics
    - Output: new data production, always a new resource
    - InOut: in-place mutation, exclusive access
    - Config: immutable engine settings, flat namespace
    - RULE: Same name as both Input and Output → build error (with example of error)
    - When to use which: decision guide
    - Parallelism implications of each binding kind
    - Debug-mode warning: InOut without writes
    - Common patterns and anti-patterns
    - Comparison with old design (brief note: "GEF originally considered State/View/Variable but evolved to the current model for clarity")
  - Running example: Extend blur module, add threshold module with InOut, add config

  **Must NOT do**:
  - Explain Pipeline/Flow composition yet
  - Deep dive into scheduler parallelism logic
  - Encourage workarounds for the same-name Input+Output rule

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Core concept chapter, needs clear explanations with examples
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (sequential within Tutorial)
  - **Parallel Group**: Wave 2
  - **Blocks**: Task 6
  - **Blocked By**: Task 4

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — Binding redesign section, parallelism implications
  - `.sisyphus/drafts/terminology-reference.md` — Binding definitions

  **Acceptance Criteria**:

  - [ ] File exists with ≥150 lines
  - [ ] Contains ≥3 C++ code blocks
  - [ ] Contains binding comparison table
  - [ ] Mentions all 4 binding kinds (Input, Output, InOut, Config)
  - [ ] Explains same-name Input+Output build error
  - [ ] Zero banned terminology

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Bindings chapter covers all 4 kinds
    Tool: Bash
    Steps:
      1. test -f docs/guide/03-bindings.md
      2. wc -l < docs/guide/03-bindings.md → Assert ≥150
      3. grep -c '```cpp\|```c++' docs/guide/03-bindings.md → Assert ≥3
      4. grep -c "GEF_INPUT\|GEF_OUTPUT\|GEF_INOUT\|GEF_CONFIG" docs/guide/03-bindings.md → Assert ≥4
      5. grep -ci "build error\|BindingConflict" docs/guide/03-bindings.md → Assert ≥1
      6. grep -c "State binding\|View binding\|Variable binding" docs/guide/03-bindings.md → Assert 0
    Expected Result: Complete bindings reference with all 4 kinds
  ```

  **Commit**: YES (groups with Task 4)
  - Message: `docs: add tutorial bindings chapter`
  - Files: `docs/guide/03-bindings.md`

---

- [x] 6. Tutorial: 04-composing-pipelines.md — Sequential Composition

  **What to do**:
  - Write `docs/guide/04-composing-pipelines.md`
  - Content:
    - What is a Pipeline (sequential chain of modules)
    - YAML config format for Pipeline:
      - `type: pipeline`, `modules:` list, `inputs:`, `outputs:`, `configs:`
    - Implicit wiring: name matching across modules
    - Skip connections: A's output reaching C even if B doesn't consume it
    - Variable list semantics: maintained as modules execute sequentially
    - Explicit wiring override: arrow syntax (`blur.blurred -> threshold.image`)
    - Structured wiring alternative: `{from: ..., to: ...}`
    - `inputs.` and `outputs.` interface boundary prefix
    - Name collision: last-writer-wins with warning (with example)
    - Config at Pipeline level: inherited by children, manual wiring for different names
    - Complete YAML example: image processing pipeline (blur → threshold → edge detect)
    - Complete C++ modules that compose into this pipeline
  - Running example: Build a 3-module image processing pipeline

  **Must NOT do**:
  - Explain Flow/DAG (next chapter)
  - Explain Batch (chapter 6)
  - Use `stages:` in YAML (use `modules:`)

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Tutorial chapter with YAML and C++ examples
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (sequential within Tutorial)
  - **Parallel Group**: Wave 2
  - **Blocks**: Task 7
  - **Blocked By**: Task 5

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — YAML format, wiring rules, Pipeline semantics
  - `.sisyphus/drafts/terminology-reference.md` — YAML field names

  **Acceptance Criteria**:

  - [ ] File exists with ≥150 lines
  - [ ] Contains ≥2 C++ code blocks
  - [ ] Contains ≥2 YAML code blocks
  - [ ] Uses `modules:` not `stages:` in all YAML
  - [ ] Shows `inputs.` and `outputs.` prefix
  - [ ] Explains implicit wiring and explicit override
  - [ ] Zero banned terminology

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Pipeline chapter has YAML and C++ examples
    Tool: Bash
    Steps:
      1. test -f docs/guide/04-composing-pipelines.md
      2. wc -l < docs/guide/04-composing-pipelines.md → Assert ≥150
      3. grep -c '```cpp\|```c++' docs/guide/04-composing-pipelines.md → Assert ≥2
      4. grep -c '```yaml' docs/guide/04-composing-pipelines.md → Assert ≥2
      5. grep -c 'stages:' docs/guide/04-composing-pipelines.md → Assert 0 (must use modules:)
      6. grep -c 'modules:' docs/guide/04-composing-pipelines.md → Assert ≥1
      7. grep -c 'inputs\.\|outputs\.' docs/guide/04-composing-pipelines.md → Assert ≥1
    Expected Result: Pipeline composition with proper YAML terminology
  ```

  **Commit**: YES (groups with Task 7)
  - Message: `docs: add tutorial pipeline and flow chapters`
  - Files: `docs/guide/04-composing-pipelines.md`

---

- [x] 7. Tutorial: 05-flows-and-parallelism.md — DAG Composition

  **What to do**:
  - Write `docs/guide/05-flows-and-parallelism.md`
  - Content:
    - What is a Flow (DAG of modules, parallel where possible)
    - Flow vs Pipeline: when to use which (decision guide)
    - YAML config for Flow: same unified format, `type: flow`
    - Explicit wiring in Flow: how edges define the DAG
    - Parallel execution: modules with satisfied inputs run concurrently
    - Name collision in Flow: error unless explicit wiring
    - Multi-file composition: `include:` for referencing sub-modules
    - Nesting: Flow can contain Pipeline, Pipeline can contain Flow
    - Complete YAML example: branching flow (preprocess → parallel branches → merge)
    - ASCII art showing DAG execution order
    - How the scheduler determines parallelism (from binding declarations)

  **Must NOT do**:
  - Explain Batch (next chapter)
  - Deep dive into scheduler algorithms
  - Use Mermaid diagrams

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Important concept chapter with diagrams
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (sequential within Tutorial)
  - **Parallel Group**: Wave 2
  - **Blocks**: Task 8
  - **Blocked By**: Task 6

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — Flow semantics, YAML format, multi-file includes
  - `docs/guide/04-composing-pipelines.md` — Previous chapter's Pipeline examples (to contrast with)

  **Acceptance Criteria**:

  - [ ] File exists with ≥150 lines
  - [ ] Contains ≥1 C++ code block
  - [ ] Contains ≥2 YAML code blocks
  - [ ] Contains ASCII art diagram of DAG
  - [ ] Contains Pipeline vs Flow comparison/decision guide
  - [ ] Shows `include:` for multi-file composition
  - [ ] Zero Mermaid, zero banned terms

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Flow chapter with DAG examples
    Tool: Bash
    Steps:
      1. test -f docs/guide/05-flows-and-parallelism.md
      2. wc -l < docs/guide/05-flows-and-parallelism.md → Assert ≥150
      3. grep -c '```yaml' docs/guide/05-flows-and-parallelism.md → Assert ≥2
      4. grep -c 'include:' docs/guide/05-flows-and-parallelism.md → Assert ≥1
      5. grep -c '```mermaid' docs/guide/05-flows-and-parallelism.md → Assert 0
      6. grep -ci "pipeline.*flow\|flow.*pipeline\|when to use" docs/guide/05-flows-and-parallelism.md → Assert ≥1
    Expected Result: Complete Flow tutorial with DAG diagrams
  ```

  **Commit**: YES (groups with Task 6)
  - Message: `docs: add tutorial flow and parallelism chapter`
  - Files: `docs/guide/05-flows-and-parallelism.md`

---

- [x] 8. Tutorial: 06-batch-processing.md — Data-Parallel Execution

  **What to do**:
  - Write `docs/guide/06-batch-processing.md`
  - Content:
    - What is a Batch module (wrapper that parallelizes over a collection)
    - Transparency: inner module sees single element, doesn't know about batching
    - Split strategies: batch_tensor_dim (dimension-based), batch_sequence (list-based), custom module
    - Gather strategies: mirror of split
    - YAML config for Batch: `type: batch`, `inner:`, `split:`, `gather:`, `mapping:`
    - Complete YAML example: video frame processor (batch over frames)
    - How scheduler distributes batch elements across threads
    - Race detection: error on overlapping InOut regions
    - Edge case: empty collection (zero iterations, silent success)
    - When to use Batch vs manual parallel implementation

  **Must NOT do**:
  - Design custom split module API in detail
  - Explain scheduler internals
  - Use banned terminology

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Concept + YAML + examples
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (sequential within Tutorial)
  - **Parallel Group**: Wave 2
  - **Blocks**: Task 9
  - **Blocked By**: Task 7

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — Batch design section

  **Acceptance Criteria**:

  - [ ] File exists with ≥120 lines
  - [ ] Contains ≥1 C++ code block
  - [ ] Contains ≥2 YAML code blocks
  - [ ] Mentions split and gather strategies
  - [ ] Covers empty collection edge case
  - [ ] Zero banned terminology

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Batch chapter covers core concepts
    Tool: Bash
    Steps:
      1. test -f docs/guide/06-batch-processing.md
      2. wc -l < docs/guide/06-batch-processing.md → Assert ≥120
      3. grep -c '```yaml' docs/guide/06-batch-processing.md → Assert ≥2
      4. grep -ci "split\|gather" docs/guide/06-batch-processing.md → Assert ≥2
      5. grep -ci "empty\|zero" docs/guide/06-batch-processing.md → Assert ≥1
    Expected Result: Batch processing tutorial complete
  ```

  **Commit**: YES (groups with Task 9)
  - Message: `docs: add tutorial batch and resource chapters`
  - Files: `docs/guide/06-batch-processing.md`

---

- [x] 9. Tutorial: 07-resource-management.md — How Memory Works

  **What to do**:
  - Write `docs/guide/07-resource-management.md`
  - Content (researcher perspective, not internals):
    - The ResourcePool: who owns the data (the pool, not you)
    - Managed pool: automatic reference counting, data released when no one needs it
    - Unmanaged pool: System-controlled persistent data (for advanced use cases)
    - How bindings relate to resources: Input/Output/InOut = your view of pool-managed data
    - Memory layout: SoA vs AoS (researcher can specify per resource via descriptor)
    - Zero-copy views: when external libraries can access data without copies
    - The borrow model: multiple readers OR single writer
    - What happens when a module finishes: resources may be released (managed pool)
    - Opaque handles: you don't get raw pointers
    - Practical advice: let the pool manage memory, don't try to work around it

  **Must NOT do**:
  - Explain ResourcePool internal implementation
  - Design the descriptor API in detail
  - Use banned terminology (especially old scope model: Consistent, promote())

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Conceptual chapter with practical advice
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (sequential within Tutorial)
  - **Parallel Group**: Wave 2
  - **Blocks**: Task 10
  - **Blocked By**: Task 8

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — ResourcePool redesign, managed/unmanaged pools

  **Acceptance Criteria**:

  - [ ] File exists with ≥100 lines
  - [ ] Contains ≥1 C++ code block
  - [ ] Explains managed vs unmanaged pool
  - [ ] Mentions borrow model (multiple readers / single writer)
  - [ ] Zero banned terminology (especially: Consistent scope, promote())

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Resource chapter covers pool types
    Tool: Bash
    Steps:
      1. test -f docs/guide/07-resource-management.md
      2. wc -l < docs/guide/07-resource-management.md → Assert ≥100
      3. grep -ci "managed\|unmanaged" docs/guide/07-resource-management.md → Assert ≥2
      4. grep -ci "borrow\|reader\|writer" docs/guide/07-resource-management.md → Assert ≥1
      5. grep -c "Consistent scope\|promote()" docs/guide/07-resource-management.md → Assert 0
    Expected Result: Resource management from researcher perspective
  ```

  **Commit**: YES (groups with Task 8)
  - Message: `docs: add tutorial resource management chapter`
  - Files: `docs/guide/07-resource-management.md`

---

- [x] 10. Tutorial: 08-debugging.md — Validation and Debug Mode

  **What to do**:
  - Write `docs/guide/08-debugging.md`
  - Content:
    - The 3 validation phases: Build-time, Schedule-time, Runtime
    - Common errors and what they mean:
      - BindingConflict (same name Input+Output): show exact scenario and fix
      - UnresolvedBinding (input not connected): show scenario and fix
      - TypeMismatch (wrong type): show scenario and fix
      - InOutRace (parallel InOut): show scenario and fix
      - CyclicDependency: show scenario and fix
    - Debug mode features:
      - InOut write detection (warning if InOut declared but never written)
      - Debug-only profiling: module execution time, memory usage
    - How to enable debug mode (build configuration)
    - Reading validation error messages
    - Tips for debugging common mistakes

  **Must NOT do**:
  - Explain validation internals
  - Design error message format in detail
  - Promise specific profiling APIs

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Practical debugging guide
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (sequential within Tutorial)
  - **Parallel Group**: Wave 2
  - **Blocks**: Task 11
  - **Blocked By**: Task 9

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — Validation error list, debug mode behavior

  **Acceptance Criteria**:

  - [ ] File exists with ≥120 lines
  - [ ] Contains ≥3 error scenario examples
  - [ ] Mentions all 3 validation phases
  - [ ] Explains debug mode
  - [ ] Zero banned terminology

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Debugging chapter covers validation
    Tool: Bash
    Steps:
      1. test -f docs/guide/08-debugging.md
      2. wc -l < docs/guide/08-debugging.md → Assert ≥120
      3. grep -ci "BindingConflict\|UnresolvedBinding\|TypeMismatch\|InOutRace\|CyclicDependency" docs/guide/08-debugging.md → Assert ≥3
      4. grep -ci "build.time\|schedule.time\|runtime" docs/guide/08-debugging.md → Assert ≥2
      5. grep -ci "debug" docs/guide/08-debugging.md → Assert ≥2
    Expected Result: Comprehensive debugging guide
  ```

  **Commit**: YES (groups with Task 11)
  - Message: `docs: add tutorial debugging chapter`
  - Files: `docs/guide/08-debugging.md`

---

- [x] 11. Tutorial: 09-external-libraries.md — Using Eigen and Beyond

  **What to do**:
  - Write `docs/guide/09-external-libraries.md`
  - Content:
    - The adapter pattern: how external libraries integrate with GEF's ResourcePool
    - Zero-copy views: when possible, external libraries get direct memory access
    - Copy fallback: when alignment/stride don't match, explicit copy with profiling log
    - Eigen integration (the primary example):
      - How to use Eigen::Map with GEF tensors
      - Example module that uses Eigen for matrix operations
    - The open adapter pattern: how the framework is extensible for any library
    - Future adapters (mention only, no design): OpenCV, ArrayFire
    - Guidelines for writing your own adapter

  **Must NOT do**:
  - Design adapters for OpenCV or ArrayFire
  - Explain GPU memory management
  - Deep dive into adapter internals beyond the pattern

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Technical tutorial with library integration examples
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (sequential within Tutorial)
  - **Parallel Group**: Wave 2
  - **Blocks**: Task 12
  - **Blocked By**: Task 10

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — External library decisions (Eigen first, open adapter pattern)

  **Acceptance Criteria**:

  - [ ] File exists with ≥100 lines
  - [ ] Contains ≥2 C++ code blocks (Eigen example)
  - [ ] Explains adapter pattern
  - [ ] Mentions zero-copy and copy fallback
  - [ ] Zero banned terminology

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: External libraries chapter has Eigen example
    Tool: Bash
    Steps:
      1. test -f docs/guide/09-external-libraries.md
      2. wc -l < docs/guide/09-external-libraries.md → Assert ≥100
      3. grep -c '```cpp\|```c++' docs/guide/09-external-libraries.md → Assert ≥2
      4. grep -ci "eigen" docs/guide/09-external-libraries.md → Assert ≥2
      5. grep -ci "adapter\|zero.copy" docs/guide/09-external-libraries.md → Assert ≥1
    Expected Result: External libraries tutorial with Eigen focus
  ```

  **Commit**: YES (groups with Task 12)
  - Message: `docs: add tutorial external libraries chapter`
  - Files: `docs/guide/09-external-libraries.md`

---

- [x] 12. Tutorial: 10-example-projects.md — Putting It All Together

  **What to do**:
  - Write `docs/guide/10-example-projects.md`
  - Content (2-3 examples, ≤2 pages each):
    - **Example 1: Image Processing Pipeline** — Complete YAML + key modules for blur → threshold → edge detect → output
    - **Example 2: Real-Time Video Processor** — Uses Batch for frame-parallel processing, demonstrates persistent data (unmanaged pool for frame counter/stats)
    - **Example 3: Scientific Computation Flow** — DAG with branching data paths, demonstrates Flow composition with nested Pipeline
    - For each: YAML config, key module code snippets, ASCII art of the module graph
    - Summary: how these examples demonstrate the full GEF feature set

  **Must NOT do**:
  - Write full implementation code (key snippets only)
  - Create more than 3 examples
  - Make examples longer than ~2 pages each
  - Introduce new concepts not covered in previous chapters

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Example-heavy capstone chapter
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (sequential within Tutorial)
  - **Parallel Group**: Wave 2
  - **Blocks**: Task 25
  - **Blocked By**: Task 11

  **References**:

  **Pattern References**:
  - All previous Tutorial chapters — Examples should reference patterns taught
  - `.sisyphus/drafts/gef-book-design.md` — System examples mention (game, video, backend)

  **Acceptance Criteria**:

  - [ ] File exists with ≥120 lines
  - [ ] Contains ≥2 YAML code blocks
  - [ ] Contains ≥2 C++ code blocks
  - [ ] Contains 2-3 distinct example projects
  - [ ] Zero banned terminology

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Example projects chapter
    Tool: Bash
    Steps:
      1. test -f docs/guide/10-example-projects.md
      2. wc -l < docs/guide/10-example-projects.md → Assert ≥120
      3. grep -c '```yaml' docs/guide/10-example-projects.md → Assert ≥2
      4. grep -c '```cpp\|```c++' docs/guide/10-example-projects.md → Assert ≥2
      5. grep -ci "example\|project" docs/guide/10-example-projects.md → Assert ≥3
    Expected Result: 2-3 complete example projects
  ```

  **Commit**: YES
  - Message: `docs: add tutorial example projects chapter`
  - Files: `docs/guide/10-example-projects.md`

---

- [x] 13. Architecture: binding-model.md — Binding Design & Rationale

  **What to do**:
  - Write `docs/design/binding-model.md`
  - Content:
    - Design evolution: from Config/State/View/Variable to Input/Output/InOut/Config (and why)
    - Each binding kind: precise definition, mutability, access semantics, lifetime
    - The same-name Input+Output rule: rationale (tracking ambiguity prevention)
    - Parallelism derivation from binding declarations
    - InOut exclusive access semantics
    - Config flat namespace injection
    - Debug-mode InOut write detection: mechanism (runtime tracking, expensive)
    - Comparison with alternatives considered (functional immutable-in/new-out, auto-new-allocation)
    - Why in-place mutation is necessary (large tensors, video frames, GPU buffers)
    - Relationship to ResourcePool (bindings are views into pool-managed resources)
    - Proxy types design rationale (auto safety)
    - State removal rationale (Config + InOut covers all use cases)

  **Must NOT do**:
  - Design the proxy type API in detail
  - Implement the binding system
  - Use old binding terminology except in the "design evolution" section

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Deep design rationale document
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 1 (with Task 2) after Task 1
  - **Blocks**: Tasks 14, 15, 17, 18, 19
  - **Blocked By**: Task 1

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — Binding redesign sections (Rounds 3-4)
  - `docs/data_system.md` — OLD design (reference for "evolution" section only, NOT for current design)

  **Acceptance Criteria**:

  - [ ] File exists with ≥150 lines
  - [ ] Contains design evolution section
  - [ ] Contains all 4 binding kinds with definitions
  - [ ] Explains State removal rationale
  - [ ] Contains parallelism implications table
  - [ ] Old terminology only appears in "evolution" context

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Binding model architecture document
    Tool: Bash
    Steps:
      1. test -f docs/design/binding-model.md
      2. wc -l < docs/design/binding-model.md → Assert ≥150
      3. grep -ci "Input\|Output\|InOut\|Config" docs/design/binding-model.md → Assert ≥8
      4. grep -ci "evolution\|history\|originally" docs/design/binding-model.md → Assert ≥1
      5. grep -ci "parallel" docs/design/binding-model.md → Assert ≥1
    Expected Result: Complete binding model design document
  ```

  **Commit**: YES
  - Message: `docs: add architecture binding model design`
  - Files: `docs/design/binding-model.md`

---

- [x] 14. Architecture: module-system.md — Module Types & Composition

  **What to do**:
  - Write `docs/design/module-system.md`
  - Content:
    - Module taxonomy: Atomic, Pipeline, Flow, Batch (+ Branch at System level)
    - Composite pattern: all modules share the same interface
    - Atomic modules: .cpp → .so, macro declarations, execute(Context&)
    - Pipeline: sequential, variable list semantics, implicit wiring, skip connections
    - Flow: DAG, explicit wiring, parallel execution
    - Batch: wrapper, transparent to inner, split/gather strategies
    - Branch: System-level (future), not a module
    - Nesting rules: any module can contain any module
    - Interface contract: every module MUST declare inputs/outputs/configs
    - Design rationale for unified module abstraction
    - Relationship to other components (scheduler uses module declarations, validation checks module graph)

  **Must NOT do**:
  - Use "Stage" terminology (except noting it was replaced)
  - Design Branch module (it's System-level and future)

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Core architecture document
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (Architecture track, after Task 13)
  - **Blocks**: Tasks 15, 17, 18, 24
  - **Blocked By**: Task 13

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — Module taxonomy, Batch design, YAML format
  - `docs/module_system.md` — OLD design (reference for evolution context only)

  **Acceptance Criteria**:

  - [ ] File exists with ≥180 lines
  - [ ] Covers all 4 module types
  - [ ] Explains composite pattern
  - [ ] Contains interface contract definition
  - [ ] Zero instances of "Stage" as primary terminology

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Module system architecture document
    Tool: Bash
    Steps:
      1. test -f docs/design/module-system.md
      2. wc -l < docs/design/module-system.md → Assert ≥180
      3. grep -ci "Atomic\|Pipeline\|Flow\|Batch" docs/design/module-system.md → Assert ≥8
      4. grep -ci "composite\|composition" docs/design/module-system.md → Assert ≥1
    Expected Result: Complete module system design
  ```

  **Commit**: YES (groups with Task 15)
  - Message: `docs: add architecture module system and plugin design`
  - Files: `docs/design/module-system.md`

---

- [x] 15. Architecture: plugin-architecture.md — Shared Libraries & Discovery

  **What to do**:
  - Write `docs/design/plugin-architecture.md`
  - Content:
    - Plugin model: each atomic module = .cpp → .so/.dylib
    - Build model: modify one file, rebuild one .so, program immediately runnable
    - Metadata exposure: query function pattern (extern "C" gef_get_metadata())
    - What the macros generate behind the scenes
    - Module discovery: directory scan for .so files
    - Loading protocol: dlopen → gef_get_metadata → register → ready for scheduling
    - API versioning: how to handle metadata struct evolution
    - Cross-platform considerations (ELF, Mach-O, PE)
    - Why query function over binary sections (simplicity, cross-platform)
    - Security considerations: running code from loaded plugins
    - Relationship to YAML config: config references module types, engine loads matching .so

  **Must NOT do**:
  - Design the full ABI in detail
  - Implement the loading system

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Technical architecture document
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (Architecture track, parallel with Tutorial)
  - **Blocks**: Task 17, 24
  - **Blocked By**: Tasks 13, 14

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — Plugin architecture decisions

  **External References**:
  - LLVM PassPlugin pattern — reference for query function approach
  - GStreamer plugin model — reference for discovery pattern

  **Acceptance Criteria**:

  - [ ] File exists with ≥120 lines
  - [ ] Explains query function pattern
  - [ ] Describes discovery mechanism
  - [ ] Mentions cross-platform considerations
  - [ ] Contains C++ code showing macro expansion

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Plugin architecture document
    Tool: Bash
    Steps:
      1. test -f docs/design/plugin-architecture.md
      2. wc -l < docs/design/plugin-architecture.md → Assert ≥120
      3. grep -ci "dlopen\|shared library\|\.so\|\.dylib" docs/design/plugin-architecture.md → Assert ≥2
      4. grep -ci "gef_get_metadata\|query function\|extern" docs/design/plugin-architecture.md → Assert ≥1
      5. grep -ci "discovery\|scan" docs/design/plugin-architecture.md → Assert ≥1
    Expected Result: Complete plugin architecture design
  ```

  **Commit**: YES (groups with Task 14)
  - Message: `docs: add architecture plugin design`
  - Files: `docs/design/plugin-architecture.md`

---

- [x] 16. Architecture: resource-pool.md — Ownership, Pools & Memory

  **What to do**:
  - Write `docs/design/resource-pool.md`
  - Content:
    - Single ownership principle: ResourcePool owns all data
    - Two pool types: Managed (refcounted, automatic) vs Unmanaged (System-controlled)
    - Resource lifecycle (FSM): Unallocated → Allocated → Bound → Borrowed → Released
    - Opaque handles: no raw pointer exposure
    - Descriptor: dtype, shape, layout (SoA/AoS), stride, alignment
    - Borrow rules: multiple reads OR single exclusive write
    - Memory layout options: SoA vs AoS per resource
    - Zero-copy views for external libraries (conditions and fallback)
    - Relationship to bindings: Input/Output/InOut = views into pool resources
    - How managed pool determines when to release (refcount from active consumers)
    - How unmanaged pool works (System explicitly allocates/releases)
    - How modules can specify which pool to use (advanced manual override)
    - Design evolution: old 3-scope model (Module/Consistent/Global) replaced by 2-pool model

  **Must NOT do**:
  - Design the allocator internals
  - Use old scope terminology except in evolution section
  - Design GPU memory management (future)

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Core architecture document
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (Architecture track)
  - **Blocks**: Tasks 17, 18, 24
  - **Blocked By**: Task 13

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — ResourcePool redesign
  - `docs/resource_pool.md` — OLD design (evolution context only)

  **Acceptance Criteria**:

  - [ ] File exists with ≥150 lines
  - [ ] Explains both pool types (Managed and Unmanaged)
  - [ ] Contains resource lifecycle FSM (ASCII art)
  - [ ] Explains borrow rules
  - [ ] Old terminology only in evolution section

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Resource pool architecture document
    Tool: Bash
    Steps:
      1. test -f docs/design/resource-pool.md
      2. wc -l < docs/design/resource-pool.md → Assert ≥150
      3. grep -ci "managed\|unmanaged" docs/design/resource-pool.md → Assert ≥4
      4. grep -ci "borrow\|ownership" docs/design/resource-pool.md → Assert ≥2
      5. grep -c "Consistent scope" docs/design/resource-pool.md | head -1 (should be 0 or only in evolution)
    Expected Result: Complete resource pool design with FSM
  ```

  **Commit**: YES
  - Message: `docs: add architecture resource pool design`
  - Files: `docs/design/resource-pool.md`

---

- [x] 17. Architecture: yaml-config.md — Configuration Format Specification

  **What to do**:
  - Write `docs/design/yaml-config.md`
  - Content:
    - Why YAML (comparison with TOML and JSON for DAG expression)
    - Complete YAML schema specification:
      - Top-level `module:` object
      - `name:`, `type:` (pipeline | flow | batch)
      - `inputs:`, `outputs:`, `configs:` (mandatory interface)
      - `modules:` (child modules list)
      - `wiring:` (explicit connections)
      - `include:` (multi-file references, relative to including file)
      - Batch-specific: `inner:`, `split:`, `gather:`, `mapping:`
    - Wiring rules:
      - Implicit wiring (name matching)
      - Explicit wiring: arrow syntax (`a.x -> b.y`) and structured format
      - `inputs.` and `outputs.` prefix for interface boundary
    - Pipeline-specific: last-writer-wins with warning
    - Flow-specific: ambiguous name collision = error
    - Config inheritance: Pipeline/Flow-level configs injected into children
    - Config key collision disambiguation: qualified names at System level
    - Multi-file include: path resolution (relative to including file)
    - Complete reference examples for each module type

  **Must NOT do**:
  - Design custom DSL (future)
  - Use `stages:` anywhere

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Specification document with examples
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (Architecture track)
  - **Blocks**: Task 24
  - **Blocked By**: Tasks 13, 14, 15

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — YAML format decisions, wiring rules

  **Acceptance Criteria**:

  - [ ] File exists with ≥200 lines
  - [ ] Contains ≥5 YAML code blocks (one per module type + wiring examples)
  - [ ] Specifies all YAML fields
  - [ ] Zero instances of `stages:` in YAML examples
  - [ ] Explains include path resolution

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: YAML config specification
    Tool: Bash
    Steps:
      1. test -f docs/design/yaml-config.md
      2. wc -l < docs/design/yaml-config.md → Assert ≥200
      3. grep -c '```yaml' docs/design/yaml-config.md → Assert ≥5
      4. grep -c 'stages:' docs/design/yaml-config.md → Assert 0
      5. grep -c 'modules:' docs/design/yaml-config.md → Assert ≥3
      6. grep -ci "include\|relative" docs/design/yaml-config.md → Assert ≥1
    Expected Result: Complete YAML specification
  ```

  **Commit**: YES
  - Message: `docs: add architecture YAML config specification`
  - Files: `docs/design/yaml-config.md`

---

- [x] 18. Architecture: scheduler.md — DAG Analysis & Execution

  **What to do**:
  - Write `docs/design/scheduler.md`
  - Content:
    - Two-level architecture: Static scheduler + Dynamic scheduler
    - Static scheduler: builds DAG from binding declarations, topological sort, parallel group identification
    - Dynamic scheduler: handles runtime decisions (future Branch routing), invokes static scheduler for sub-graphs
    - How parallelism is determined from binding types:
      - Input = read (safe to parallelize)
      - Output = new (no conflict)
      - InOut = exclusive (serializes)
    - Scheduler as abstract interface: engineer chooses/configures/extends
    - Deterministic tie-breaking for parallel groups (topological order + name)
    - Batch element distribution across threads
    - Relationship to validation (scheduler produces schedule, validation checks it)
    - Future work: dynamic branching at System level
    - No internal algorithm specifics (that's implementation)

  **Must NOT do**:
  - Design scheduler algorithm internals
  - Specify thread pool implementation
  - Design the Branch routing mechanism (future)

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Architecture document
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (Architecture track)
  - **Blocks**: Task 19, 24
  - **Blocked By**: Tasks 13, 14, 16

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — Scheduler architecture decisions

  **Acceptance Criteria**:

  - [ ] File exists with ≥120 lines
  - [ ] Explains two-level architecture (static + dynamic)
  - [ ] Describes how binding types determine parallelism
  - [ ] Mentions engineer configurability
  - [ ] Zero banned terminology

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Scheduler architecture document
    Tool: Bash
    Steps:
      1. test -f docs/design/scheduler.md
      2. wc -l < docs/design/scheduler.md → Assert ≥120
      3. grep -ci "static\|dynamic" docs/design/scheduler.md → Assert ≥4
      4. grep -ci "parallel\|DAG" docs/design/scheduler.md → Assert ≥2
    Expected Result: Complete scheduler design
  ```

  **Commit**: YES (groups with Task 19)
  - Message: `docs: add architecture scheduler and validation design`
  - Files: `docs/design/scheduler.md`

---

- [x] 19. Architecture: validation-layer.md — Safety & Error Detection

  **What to do**:
  - Write `docs/design/validation-layer.md`
  - Content:
    - 3 validation phases: Build-time, Schedule-time, Runtime
    - Complete error type table (11 errors with phase, description, trigger, suggested fix)
    - Build-time checks: BindingConflict, UnresolvedBinding, TypeMismatch, UnusedOutput, CyclicDependency
    - Schedule-time checks: InOutRace, InOutReadRace, BatchOverlap
    - Runtime checks (debug only): InOutNoWrite, UseAfterFree, BackendMismatch
    - Debug-only profiling: what's tracked, zero overhead in release
    - Relationship to scheduler (validation runs on the produced schedule)
    - Error reporting: what the user sees, how to interpret errors
    - Design rationale: why 3 phases (catch errors as early as possible)

  **Must NOT do**:
  - Design error message formatting details
  - Implement the validation system

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Architecture document with tables
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (Architecture track)
  - **Blocks**: Task 24
  - **Blocked By**: Tasks 13, 18

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — Validation error list, 3-phase model

  **Acceptance Criteria**:

  - [ ] File exists with ≥120 lines
  - [ ] Contains error type table with all 11 errors
  - [ ] Explains 3 validation phases
  - [ ] Mentions debug-only profiling
  - [ ] Zero banned terminology

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Validation layer architecture document
    Tool: Bash
    Steps:
      1. test -f docs/design/validation-layer.md
      2. wc -l < docs/design/validation-layer.md → Assert ≥120
      3. grep -ci "BindingConflict\|UnresolvedBinding\|TypeMismatch\|InOutRace\|CyclicDependency\|BatchOverlap\|InOutNoWrite\|UseAfterFree\|BackendMismatch\|InOutReadRace\|UnusedOutput" docs/design/validation-layer.md → Assert ≥8
      4. grep -ci "build.time\|schedule.time\|runtime" docs/design/validation-layer.md → Assert ≥3
    Expected Result: Complete validation layer design
  ```

  **Commit**: YES (groups with Task 18)
  - Message: `docs: add architecture validation layer design`
  - Files: `docs/design/validation-layer.md`

---

- [x] 20. Architecture: system.md — The Top-Level Orchestrator

  **What to do**:
  - Write `docs/design/system.md`
  - Content:
    - System = the program (no higher-level concept)
    - Responsibilities: owns ResourcePools, loads modules, runs scheduler, manages lifecycle
    - Two personas: Researcher (writes modules) vs Engineer (builds System)
    - System lifecycle: setup → configure → run (possibly looping) → teardown
    - ResourcePool ownership: System creates and owns both Managed and Unmanaged pools
    - Scheduler selection: System configures which scheduler to use
    - Module loading: System triggers directory scan and plugin loading
    - Branch logic: handled at System level (future), not in modules
    - System examples: game engine, video processor, backend server
    - How System relates to YAML config (System reads config and orchestrates)
    - Persistent data: managed by System through Unmanaged ResourcePool

  **Must NOT do**:
  - Design the System API in detail
  - Implement Branch routing
  - Design System configuration format

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Core architecture document
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (Architecture track)
  - **Blocks**: Task 24
  - **Blocked By**: Tasks 13, 14, 16, 18

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — System concept, two personas, ResourcePool ownership

  **Acceptance Criteria**:

  - [ ] File exists with ≥120 lines
  - [ ] Explains System as top-level (no higher concept)
  - [ ] Describes two personas (Researcher vs Engineer)
  - [ ] Covers System lifecycle
  - [ ] Mentions System examples (game, video, server)

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: System architecture document
    Tool: Bash
    Steps:
      1. test -f docs/design/system.md
      2. wc -l < docs/design/system.md → Assert ≥120
      3. grep -ci "researcher\|engineer" docs/design/system.md → Assert ≥4
      4. grep -ci "lifecycle\|setup\|teardown" docs/design/system.md → Assert ≥1
      5. grep -ci "game\|video\|server" docs/design/system.md → Assert ≥1
    Expected Result: Complete System architecture
  ```

  **Commit**: YES
  - Message: `docs: add architecture system design`
  - Files: `docs/design/system.md`

---

- [x] 21. Architecture: external-adapters.md — Library Integration Pattern

  **What to do**:
  - Write `docs/design/external-adapters.md`
  - Content:
    - Open adapter pattern: extensible framework for any external library
    - Adapter responsibilities: bridge between ResourcePool memory and library-native types
    - Zero-copy view mechanism: when alignment/stride match, create views directly
    - Copy fallback: when conditions aren't met, explicit copy + profiling log
    - Eigen adapter (primary example): Eigen::Map over ResourcePool-owned tensors
    - How adapters handle memory layout (SoA/AoS) differences
    - Future adapters: OpenCV (cv::Mat views), ArrayFire (af::array), mention only
    - GPU memory adapters: future work (device pointers, synchronization)
    - Guidelines for creating custom adapters

  **Must NOT do**:
  - Design OpenCV or ArrayFire adapters
  - Design GPU memory adapters
  - Implement the Eigen adapter

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Architecture document
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (Architecture track)
  - **Blocks**: Task 24
  - **Blocked By**: Task 16

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — External library decisions

  **Acceptance Criteria**:

  - [ ] File exists with ≥100 lines
  - [ ] Explains adapter pattern
  - [ ] Contains Eigen adapter example
  - [ ] Mentions zero-copy and copy fallback
  - [ ] Does NOT design OpenCV/ArrayFire adapters in detail

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: External adapters architecture document
    Tool: Bash
    Steps:
      1. test -f docs/design/external-adapters.md
      2. wc -l < docs/design/external-adapters.md → Assert ≥100
      3. grep -ci "adapter\|pattern" docs/design/external-adapters.md → Assert ≥3
      4. grep -ci "eigen" docs/design/external-adapters.md → Assert ≥2
      5. grep -ci "zero.copy" docs/design/external-adapters.md → Assert ≥1
    Expected Result: Adapter pattern documentation
  ```

  **Commit**: YES (groups with Task 22)
  - Message: `docs: add architecture external adapters and error handling`
  - Files: `docs/design/external-adapters.md`

---

- [x] 22. Architecture: error-handling.md — Error Philosophy & Strategy

  **What to do**:
  - Write `docs/design/error-handling.md`
  - Content:
    - Hybrid strategy: exceptions + Result types
    - When to use exceptions: truly unrecoverable (OOM, corrupted state)
    - When to use Result types: expected errors (validation failures, borrow violations)
    - Why hybrid: C++ ecosystem pragmatism, performance considerations
    - How validation errors are surfaced (Result types at API boundary)
    - How runtime panics work (exceptions for catastrophic failures)
    - Error propagation patterns
    - Relationship to validation layer (validation produces errors → Result types)
    - Debug mode: additional error context and stack traces

  **Must NOT do**:
  - Design the Result<T> type's complete API
  - Design exception class hierarchy
  - Over-specify error message formats

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Architecture philosophy document
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (Architecture track)
  - **Blocks**: Task 24
  - **Blocked By**: Task 13

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — Error handling decision

  **Acceptance Criteria**:

  - [ ] File exists with ≥80 lines
  - [ ] Explains hybrid strategy (exceptions + Result types)
  - [ ] Defines when to use each pattern
  - [ ] Mentions debug mode enhancements

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Error handling architecture document
    Tool: Bash
    Steps:
      1. test -f docs/design/error-handling.md
      2. wc -l < docs/design/error-handling.md → Assert ≥80
      3. grep -ci "exception\|Result" docs/design/error-handling.md → Assert ≥3
      4. grep -ci "hybrid" docs/design/error-handling.md → Assert ≥1
    Expected Result: Error handling philosophy documented
  ```

  **Commit**: YES (groups with Task 21)
  - Message: `docs: add architecture error handling design`
  - Files: `docs/design/error-handling.md`

---

- [x] 23. Architecture: development-roadmap.md — Phases & Future Work

  **What to do**:
  - Write `docs/design/development-roadmap.md`
  - Content:
    - Updated phase plan (reflecting new design):
      - Phase 1: Core ResourcePool (CPU, managed pool, basic borrow rules)
      - Phase 2: Module system (Atomic module loading, macro framework, Context API)
      - Phase 3: Binding system (Input/Output/InOut/Config, validation)
      - Phase 4: Pipeline composition (YAML parsing, implicit wiring, sequential execution)
      - Phase 5: Flow composition (DAG scheduling, parallel execution)
      - Phase 6: Batch module (split/gather, parallel map)
      - Phase 7: External adapters (Eigen first)
      - Phase 8: Unmanaged ResourcePool and System API
      - Phase 9: Debug tooling (profiling, InOut write detection, enhanced error reporting)
      - Future: GPU backends, Branch routing, GUI composition tool, custom config DSL
    - Build system notes: pixi + CMake + C++23
    - No dates or effort estimates

  **Must NOT do**:
  - Include timeline or effort estimates
  - Make promises about specific features
  - Exceed 1-2 pages

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Short planning document
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: YES
  - **Parallel Group**: Wave 2 (Architecture track)
  - **Blocks**: Task 24
  - **Blocked By**: Task 13

  **References**:

  **Pattern References**:
  - `.sisyphus/drafts/gef-book-design.md` — All design decisions
  - `docs/architecture.md` — OLD phase plan (reference only, update completely)

  **Acceptance Criteria**:

  - [ ] File exists with ≥80 lines
  - [ ] Contains ≥6 phases
  - [ ] Contains "Future" section for aspirational features
  - [ ] Does NOT contain dates or effort estimates

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Development roadmap
    Tool: Bash
    Steps:
      1. test -f docs/design/development-roadmap.md
      2. wc -l < docs/design/development-roadmap.md → Assert ≥80
      3. grep -ci "phase" docs/design/development-roadmap.md → Assert ≥6
      4. grep -ci "future" docs/design/development-roadmap.md → Assert ≥1
      5. grep -ci "week\|month\|sprint\|Q[1-4]\|2026\|2027" docs/design/development-roadmap.md → Assert 0
    Expected Result: Phase-based roadmap without dates
  ```

  **Commit**: YES
  - Message: `docs: add architecture development roadmap`
  - Files: `docs/design/development-roadmap.md`

---

- [x] 24. Architecture: overview.md — Hub Page

  **What to do**:
  - Write `docs/design/overview.md`
  - Content:
    - GEF philosophy recap (explicit control, safe concurrency, single ownership)
    - High-level system architecture (ASCII art showing all components)
    - Core concepts quick reference table
    - Reading guide: recommended order for the Architecture Reference
    - Links to ALL 11 other design files with one-line descriptions
    - How the Tutorial and Architecture Reference relate (Tutorial = how to use, Reference = how it works)
    - Glossary of key terms

  **Must NOT do**:
  - Duplicate content from deep-dive files
  - Use Mermaid diagrams

  **Recommended Agent Profile**:
  - **Category**: `writing`
    - Reason: Hub/index document
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (needs all other design files first)
  - **Parallel Group**: Wave 3
  - **Blocks**: Task 25
  - **Blocked By**: Tasks 13-23

  **References**:

  **Pattern References**:
  - All `docs/design/*.md` files — summarize and link to each
  - `.sisyphus/drafts/gef-book-design.md` — Philosophy, core concepts

  **Acceptance Criteria**:

  - [ ] File exists with ≥100 lines
  - [ ] Links to all 11 other design files
  - [ ] Contains ASCII art architecture diagram
  - [ ] Contains glossary or core concepts table
  - [ ] Zero Mermaid

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Overview hub links to all design files
    Tool: Bash
    Steps:
      1. test -f docs/design/overview.md
      2. wc -l < docs/design/overview.md → Assert ≥100
      3. for f in system.md module-system.md binding-model.md resource-pool.md scheduler.md validation-layer.md yaml-config.md plugin-architecture.md external-adapters.md error-handling.md development-roadmap.md; do grep -q "$f" docs/design/overview.md || echo "MISSING: $f"; done
      4. Assert: no MISSING output
      5. grep -c '```mermaid' docs/design/overview.md → Assert 0
    Expected Result: Hub page linking all design files
  ```

  **Commit**: YES (groups with Task 25)
  - Message: `docs: add architecture overview hub and cleanup`
  - Files: `docs/design/overview.md`

---

- [x] 25. Cleanup: Delete old docs + Update README

  **What to do**:
  - Delete the 6 obsolete design docs:
    - `docs/architecture.md`
    - `docs/module_system.md`
    - `docs/data_system.md`
    - `docs/validation_layer.md`
    - `docs/scheduler.md`
    - `docs/resource_pool.md`
  - Update `README.md` with:
    - Brief project description (what GEF is)
    - Philosophy one-liner
    - Links to both books:
      - `docs/guide/` — Tutorial for researchers
      - `docs/design/` — Architecture Reference for engineers
    - Build prerequisites (pixi, C++23)
    - Quick start pointer (link to `docs/guide/01-quick-start.md`)

  **Must NOT do**:
  - Delete files before all new docs are verified
  - Write extensive README (keep it concise, point to docs)

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: File deletion and simple README update
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (final task)
  - **Parallel Group**: Wave 3 (after all content)
  - **Blocks**: Task 26
  - **Blocked By**: Tasks 12, 24

  **References**:

  **Pattern References**:
  - `README.md` — Current content (to be replaced)
  - `.sisyphus/drafts/gef-book-design.md` — Project description

  **Acceptance Criteria**:

  - [ ] 6 old docs deleted (not found)
  - [ ] README.md updated with doc links
  - [ ] README.md ≥20 lines

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Old docs removed and README updated
    Tool: Bash
    Steps:
      1. test ! -f docs/architecture.md → Assert true
      2. test ! -f docs/module_system.md → Assert true
      3. test ! -f docs/data_system.md → Assert true
      4. test ! -f docs/validation_layer.md → Assert true
      5. test ! -f docs/scheduler.md → Assert true
      6. test ! -f docs/resource_pool.md → Assert true
      7. wc -l < README.md → Assert ≥20
      8. grep -c "docs/guide\|docs/design" README.md → Assert ≥2
    Expected Result: Clean docs directory with updated README
  ```

  **Commit**: YES
  - Message: `docs: remove obsolete design docs and update README`
  - Files: deleted docs + `README.md`

---

- [x] 26. Final compliance check

  **What to do**:
  - Run comprehensive verification across ALL new documentation:
    - All 23 files exist with ≥80 lines
    - Zero banned terminology across all files
    - Zero Mermaid diagrams
    - All YAML examples use `modules:` not `stages:`
    - Architecture overview links to all 11 design files
    - Tutorial chapters 02-09 have C++ code blocks
    - YAML-relevant chapters have YAML code blocks
    - 6 old docs are deleted
    - README updated

  **Must NOT do**:
  - Modify content (just verify)

  **Recommended Agent Profile**:
  - **Category**: `quick`
    - Reason: Automated verification only
  - **Skills**: []

  **Parallelization**:
  - **Can Run In Parallel**: NO (final verification)
  - **Parallel Group**: Wave 3 (last task)
  - **Blocks**: None
  - **Blocked By**: Task 25

  **References**:

  **Acceptance Criteria**:

  - [ ] ALL verification commands pass
  - [ ] Zero compliance failures

  **Agent-Executed QA Scenarios**:

  ```
  Scenario: Full compliance check
    Tool: Bash
    Steps:
      1. Count files in docs/guide/ → Assert 11
      2. Count files in docs/design/ → Assert 12
      3. for f in docs/guide/*.md docs/design/*.md; do lines=$(wc -l < "$f"); [ "$lines" -lt 80 ] && echo "SHORT: $f ($lines)"; done → Assert no SHORT output
      4. grep -rn "State binding\|View binding\|Variable binding\|Consistent scope\|promote()" docs/guide/ docs/design/ → Assert 0 matches
      5. grep -rn '```mermaid' docs/guide/ docs/design/ → Assert 0 matches
      6. grep -rn 'stages:' docs/guide/ docs/design/ → Assert 0 matches (within YAML blocks)
      7. for f in docs/guide/02*.md docs/guide/03*.md docs/guide/04*.md docs/guide/05*.md docs/guide/06*.md docs/guide/07*.md docs/guide/08*.md docs/guide/09*.md; do grep -q '```cpp\|```c++' "$f" || echo "NO CODE: $f"; done → Assert no NO CODE
      8. for f in docs/guide/04*.md docs/guide/05*.md docs/guide/06*.md docs/design/yaml-config.md; do grep -q '```yaml' "$f" || echo "NO YAML: $f"; done → Assert no NO YAML
      9. test ! -f docs/architecture.md && test ! -f docs/module_system.md → Assert true
      10. grep -q "docs/guide" README.md && grep -q "docs/design" README.md → Assert true
    Expected Result: All compliance checks pass
    Evidence: Terminal output captured showing all assertions passed
  ```

  **Commit**: NO (verification only)

---

## Commit Strategy

| After Task(s) | Message | Key Files |
|---|---|---|
| 1 | `docs: create directory structure and terminology reference` | dirs, terminology |
| 2-3 | `docs: add tutorial intro and quick-start chapters` | 00, 01 |
| 4-5 | `docs: add tutorial module anatomy and bindings chapters` | 02, 03 |
| 6-7 | `docs: add tutorial pipeline and flow chapters` | 04, 05 |
| 8-9 | `docs: add tutorial batch and resource chapters` | 06, 07 |
| 10-11 | `docs: add tutorial debugging and external libraries chapters` | 08, 09 |
| 12 | `docs: add tutorial example projects chapter` | 10 |
| 13 | `docs: add architecture binding model design` | binding-model |
| 14-15 | `docs: add architecture module and plugin design` | module-system, plugin |
| 16 | `docs: add architecture resource pool design` | resource-pool |
| 17 | `docs: add architecture YAML config specification` | yaml-config |
| 18-19 | `docs: add architecture scheduler and validation design` | scheduler, validation |
| 20 | `docs: add architecture system design` | system |
| 21-22 | `docs: add architecture adapters and error handling` | external-adapters, error-handling |
| 23 | `docs: add architecture development roadmap` | roadmap |
| 24-25 | `docs: add architecture overview hub, cleanup old docs, update README` | overview, deleted, README |

---

## Success Criteria

### Verification Commands
```bash
# All new files exist (23 total)
ls docs/guide/*.md | wc -l    # Expected: 11
ls docs/design/*.md | wc -l   # Expected: 12

# All old files removed
ls docs/architecture.md docs/module_system.md docs/data_system.md docs/validation_layer.md docs/scheduler.md docs/resource_pool.md 2>&1 | grep -c "No such file"   # Expected: 6

# No banned terminology
grep -rn "State binding\|View binding\|Variable binding\|Consistent scope\|promote()" docs/guide/ docs/design/ | wc -l   # Expected: 0

# No Mermaid
grep -rn '```mermaid' docs/guide/ docs/design/ | wc -l   # Expected: 0

# No stages: in YAML blocks
grep -rn 'stages:' docs/guide/ docs/design/ | wc -l   # Expected: 0

# README updated
grep -c "docs/guide\|docs/design" README.md   # Expected: ≥2
```

### Final Checklist
- [ ] All 11 Tutorial chapters present and substantive
- [ ] All 12 Architecture Reference files present and substantive
- [ ] Overview hub links to all 11 other design files
- [ ] All YAML examples use `modules:` consistently
- [ ] All C++ examples use GEF macro pattern consistently
- [ ] All diagrams are ASCII art (no Mermaid)
- [ ] All binding references use Input/Output/InOut/Config
- [ ] No superseded terminology in documentation
- [ ] README updated with links to both books
- [ ] 6 old design docs deleted
- [ ] Running example (image processing) threads through Tutorial
- [ ] Rust Book writing style maintained throughout
