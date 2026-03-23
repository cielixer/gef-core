---
name: orthodox-modern-cpp
description: Apply an aggressive Orthodox Modern C++23 subset with explicit runtime behavior and type safety.
---

# Orthodox Modern C++ Skill (C++23)

## Purpose
Use this skill to implement an explicit-runtime Orthodox Modern C++23 subset. Prefer type safety, readability, and predictable runtime behavior over hidden costs and abstraction-heavy designs.

## When This Skill Should Trigger
Use this skill when the codebase or module requires a modern C++23 approach with explicit costs and type safety, but without the baggage of older conservative Orthodox C++ or the overhead of complex standard features like exceptions and RTTI.

Do not trigger this skill for general modern C++ codebases that rely on exceptions, RTTI, or framework conventions.

## Core Orthodox C++ Principles
1. Simplicity over novelty.
   - Start from C-like solutions and add C++ features only when they clearly reduce complexity.
   - Favor code that is obvious to developers familiar with C and modern value-oriented C++.

2. Make runtime cost explicit.
   - Avoid features with hidden control flow or hidden allocation behavior.
   - Keep ownership, allocation, and lifetime visible in APIs.

3. Keep the runtime model predictable.
   - Prefer deterministic control flow and visible ownership/allocation costs.
   - Use RAII for simple scope-bound cleanup when it improves clarity, but avoid constructors/destructors that hide expensive work, allocation, or non-obvious side effects.

4. Use explicit error handling.
   - Use `std::expected` as the default explicit error model for expected failure paths.
   - Avoid exception-driven control flow.

5. Adopt a modern C++23 subset aggressively when it improves clarity, type safety, and explicitness without hiding runtime cost.
   - Do not adopt language features solely for trend alignment.

6. Prefer portability-first build choices.
   - Avoid build-system and compiler constraints that reduce portability without measurable gain.
   - Keep interop-facing headers C API compatible where practical so future bindings/FFI remain feasible.

## Allowed C++23 Subset
Zero-cost or near-zero-cost features blessed by this skill. All must respect active toolchain support.

- **Data modeling**: `struct` only — `class` keyword is forbidden. Free functions over member methods. Scoped enums. Designated initializers for aggregate construction. `std::optional` and `std::variant` for presence/sum types.
- **Views**: `std::span` and `std::string_view` for non-owning access. Views must never outlive underlying storage.
- **Error handling**: `std::expected` for all expected failure paths. Validate `std::print` availability; fall back to `std::format` plus project output sink.
- **Formatting**: `std::format` / `std::print` over `printf`, `snprintf`, and iostream.
- **Paths**: `std::filesystem::path` for all filesystem locations — never raw `std::string`/`const char*` path plumbing.
- **Diagnostics**: `std::source_location` over `__FILE__`/`__LINE__`.
- **Declarations**: AAA `auto` for locals when initializer intent is clear; explicit types where contracts or readability require them.
- **Compile-time**: `constexpr`, `consteval`, `constinit` aggressively to push work to compile time.
- **Attributes**: `[[nodiscard]]`, `[[maybe_unused]]`, `[[likely]]`, `[[unlikely]]`, `[[no_unique_address]]` aggressively. `[[assume(expr)]]` ONLY for proven hot-path invariants paired with debug assertions.
- **Control flow**: Structured bindings and if/switch initializers.
- **Generics**: Simple templates, alias templates, and concepts when generated code stays obvious.
- **Rule of 5**: Declare copy/move constructors, copy/move assignment, and destructor intent explicitly for non-trivial `struct` types.

## Ownership Ladder
Apply in order — escalate only when the simpler option is insufficient:
1. Value types, stack allocation, and non-owning views (`std::span`, `std::string_view`) FIRST.
2. Move semantics for explicit ownership transfer.
3. `std::unique_ptr` ONLY for exclusive dynamic ownership when stack allocation is insufficient.
4. `std::shared_ptr` ONLY for genuine shared lifetime where multiple components must co-own an object.
5. `std::weak_ptr` ONLY for observing shared lifetime or breaking reference cycles.

## STL Container Policy Matrix
| Family | Status | Rationale |
| :--- | :--- | :--- |
| Borrowed views (`std::span`, `std::string_view`) | Recommended | Zero-copy, zero-allocation non-owning access. |
| `std::array` | Recommended | Fixed-size, stack-allocated, cache-friendly. |
| `std::vector` | Recommended | Default growable container. Contiguous memory, cache-friendly. Prefer `std::array` when size is compile-time known; otherwise `std::vector` is the correct default. |
| `std::deque` | Restricted | Fragmented storage across multiple blocks hurts cache locality. Allow only when front/back insertion is a real, documented requirement. |
| `std::list`, `std::forward_list` | Forbidden | Node-based allocation destroys cache locality; per-element heap overhead; pointer chasing on every traversal. |
| Ordered associative (`std::map`, `std::set`, `std::multimap`, `std::multiset`) | Forbidden | Node-based allocation, poor cache locality, hidden rebalancing costs on every insert/erase. |
| Unordered associative (`std::unordered_map`, `std::unordered_set`, etc.) | Restricted | Hash table rehashing causes hidden bulk reallocation. Allowed only when hashed lookup need is proven and simpler alternatives are impractical. |
| Adaptor containers (`std::stack`, `std::queue`, `std::priority_queue`) | Restricted | Thin wrappers — cost depends on underlying container. Allowed when operation semantics are needed and underlying container + thread policy is documented. |

For ALL containers used across threads: verify concurrency model explicitly — no unsynchronized concurrent mutation. Document synchronization/ownership strategy.

## External Library Exception Boundaries
External libraries (e.g., Boost.DLL, system APIs) may throw exceptions that are outside our control. At integration boundaries:
- Catch external exceptions at the **immediate call site** and convert to `std::expected<T, Error>`.
- Never let external exceptions propagate through GEF APIs.
- Document which external calls may throw and where the catch boundary lives.

## Logging Policy
- Use the **project's logging library** (currently `spdlog`) for all runtime log output.
- `std::format` / `std::print` are for general string formatting, **not** for logging.
- Do not replace an established project logger with raw `std::print` calls.
- If no project logger exists, prefer `std::print` to stderr over iostream.

## Data Modeling
- **`struct` only** — the `class` keyword is forbidden. All types are `struct`.
- Use free functions operating on `struct` types instead of member methods.
- `virtual` destructors and methods are forbidden.

## Mandatory Rules for the Agent
- Follow the Allowed C++23 Subset, Ownership Ladder, and STL Container Policy Matrix above.
- Use `std::expected` as the primary error mechanism. Never use exceptions for expected failure paths.
- Prefer composition over inheritance. `struct` only — `class` keyword is forbidden. Free functions over member methods.
- Keep memory ownership explicit in function signatures and module boundaries.
- Document any intentional deviation from Orthodox Modern rules with rationale and trade-offs.

**NEVER:**
- `class` keyword — use `struct` for all types. Use free functions instead of member methods.
- Exceptions for control flow or expected failures.
- RTTI (`dynamic_cast`, `typeid`) for dispatch or design.
- Ranges pipelines, coroutines, or C++20 modules.
- iostream-heavy formatting where `std::format`/`std::print` suffices.
- Smart pointers as default substitute for value semantics.
- `shared_ptr` to dodge ownership modeling.
- Forbidden containers (`std::list`, `std::forward_list`, ordered associative).
- Restricted containers without explicit justification in comments (except `std::vector`, which is Recommended).
- Dangling views — `std::span`/`std::string_view` must never outlive source storage.
- `[[assume(expr)]]` on unproven invariants or outside profiled hot paths.
- Deep inheritance trees, excessive template metaprogramming, or abstraction layers that obscure runtime cost.
- Adopting C++ features without toolchain validation and clear benefit.

## Execution Workflow
1. Identify module constraints (performance, portability, toolchain, platform).
2. Select ownership and container policy BEFORE implementation (Ownership Ladder + STL Policy Matrix). Document thread-safety plan for shared containers.
3. Model data with passive `struct`, scoped enums, designated initializers, and view/value types.
4. Implement failure paths with `std::expected` before considering lower-level encodings.
5. Verify forbidden features (ranges, coroutines, modules, exceptions, RTTI, iostream) remain absent.
6. Validate interop/public header boundaries: C API-safe signatures, `extern "C"` exposure points.
7. Profile hot paths for hidden allocation or avoidable abstraction cost.
8. Add tests for success, failure, and edge-case behavior.
9. Document deviations and why they are necessary.

## PR/Review Checklist
- Is the code simple enough for C-familiar developers to follow quickly?
- Does ownership follow the ladder (Value first) and is each escalation justified?
- Does container selection comply with the STL Policy Matrix? Are restricted containers justified?
- Is thread-safety documented for every shared container usage?
- Are `struct`-only modeling (no `class`), Rule-of-5, free functions, and designated initializers respected?
- Are error paths explicit via `std::expected` without exception-driven flow?
- Are forbidden features (ranges, coroutines, modules, exceptions, RTTI) absent?
- Are interop headers C API compatible with clean `extern "C"` boundaries?
- Are hot-path allocations profiled and justified?
- Are deviations documented with trade-offs?

## Output Format Expectations for the Agent
When proposing or implementing changes, include:
1. Target module constraints (performance, portability, toolchain)
2. Chosen feature policy (allowed vs rejected features from the Allowed Subset)
3. Error-handling model (`std::expected` flow)
4. Ownership and lifetime strategy (Ladder position + justification)
5. Container choices (STL Policy Matrix compliance + restriction justifications)
6. Formatting/output strategy (`std::format`/`std::print` + portability fallback)
7. Interop header strategy (C API boundary + wrapper split)
8. Any Orthodox-rule deviations and rationale

## Source Orientation
Rooted in Orthodox C++ (Branimir Karadžić / bgfx), emphasizing simplicity, portability, and explicit costs. This skill intentionally breaks from the conservative +5-year standard adoption rule, aggressively adopting C++23 features that meet the explicitness and zero-cost criteria.
