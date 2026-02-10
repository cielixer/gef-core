# Error Handling Architecture

This document describes the hybrid error handling strategy employed by the General Engine Framework (GEF). It outlines the rationale, patterns, and mechanisms used to balance performance, safety, and developer experience (DX).

## 1. Overview

GEF adopts a hybrid error handling strategy that combines C++ exceptions with `Result<T>` types. This dual-path approach acknowledges that not all errors are created equal: some are expected parts of data processing flows, while others represent catastrophic failures of the system state or environment.

By separating "expected" errors from "exceptional" ones, GEF provides a robust framework that is both type-safe for common failure modes and performant for the happy path.

## 2. When to Use Exceptions

Exceptions in GEF are reserved for truly unrecoverable, catastrophic failures. These are events that indicate the system cannot safely continue execution or that a fundamental invariant has been violated.

Specific scenarios for exceptions include:
- **Out of Memory (OOM)**: The system cannot allocate essential resources.
- **Corrupted Internal State**: Detection of memory corruption or invalid internal pointers.
- **Catastrophic Environment Failures**: Loss of connection to essential hardware (e.g., GPU disconnect) or missing core system libraries.
- **Contract Violations**: Logic errors in the engine itself that represent a bug in GEF rather than a user error.

Exceptions should be rare. They represent the "stop the world" scenario where the current execution context must be torn down.

## 3. When to Use Result Types

For the vast majority of errors encountered during normal operation, GEF uses `Result<T>` types (similar to `std::expected` in C++23). These are used for recoverable or expected failure modes where the error is a first-class citizen of the API.

Scenarios for `Result<T>` types include:
- **Validation Failures**: Input data that does not meet the requirements of a module.
- **Borrow Violations**: Conflicts in resource access (e.g., two modules attempting to `InOut` the same data concurrently).
- **Type Mismatches**: Data flowing between modules that does not match the expected schema.
- **Configuration Errors**: Invalid parameters provided in a YAML config file.
- **Resource Timeouts**: Temporary inability to acquire a resource that might be resolved by retrying or skipping a step.

`Result<T>` types force the caller to explicitly handle or propagate the error, ensuring that failures are not accidentally ignored.

## 4. Why Hybrid?

The decision to use a hybrid approach is rooted in C++ ecosystem pragmatism.

- **Pure Result Types**: While popular in languages like Rust, pure `Result` types in C++ can be awkward due to the lack of a built-in `?` operator or similar syntax sugar (until widespread adoption of C++23's monadic operations). Forcing every single internal function to return a `Result` adds significant boilerplate and cognitive load.
- **Pure Exceptions**: Exceptions are excellent for reducing boilerplate on the happy path, but they lose type safety for expected errors. Relying solely on exceptions for validation errors makes the control flow "invisible" and often leads to performance penalties when errors occur frequently.

The hybrid model gives GEF the best of both worlds:
1. **Clarity**: API boundaries use `Result<T>` to make error conditions explicit.
2. **Performance**: The happy path avoids the overhead of `Result` checking where appropriate, and catastrophic paths use exceptions to jump directly to a recovery or shutdown point.
3. **Safety**: Expected errors are encoded in the type system, preventing them from being ignored.

## 5. Error Propagation Patterns

Errors flow through the GEF system following established patterns based on the module hierarchy.

### Result Propagation
At the boundary of `Atomic` modules and the `System` level, `Result<T>` is the standard return type. When an `Atomic` module fails during its `execute()` method, it returns a `Result` containing the error details. The `Scheduler` then receives this result and decides whether to halt the current `Flow`, trigger a retry, or log the error and continue (depending on the `System` configuration).

### Exception Propagation
Exceptions are caught at the highest possible level—typically within the `System` execution loop. When a catastrophic exception occurs, GEF performs emergency cleanup of `ResourcePool` assets, logs the stack trace (in debug mode), and gracefully shuts down the execution.

## 6. Relationship to Validation Layer

The GEF Validation Layer is the primary producer of `Result` types. It operates across three distinct phases, surfacing errors as they are detected:

1. **Build-time**: Errors like `BindingConflict`, `UnresolvedBinding`, and `TypeMismatch` are surfaced via `Result` types during YAML parsing and module loading. These prevent the `System` from even starting an execution.
2. **Schedule-time**: Errors such as `InOutRace` or `BatchOverlap` are detected by the `Scheduler` before execution begins, returned as a `Result` to the `Engineer`.
3. **Runtime**: Debug-only checks like `InOutNoWrite` or `UseAfterFree` produce `Result` types or warnings during the actual execution of modules.

## 7. Debug Mode Enhancements

In debug builds, GEF enhances error reports to facilitate rapid troubleshooting for researchers:

- **Additional Context**: Errors include the name of the module, the specific binding involved, and the YAML file location where the module was composed.
- **Stack Traces**: Exceptions and certain `Result` errors capture stack traces to pinpoint the failure in the C++ source code.
- **InOut Write Detection**: If an `InOut` binding is declared but no write is detected during execution, a warning is generated. This helps researchers identify performance optimizations (converting `InOut` to `Input`).
- **Profiling Data**: Error reports can include timing information and memory usage stats at the moment of failure.

## 8. Design Principles

The error handling architecture is guided by three core principles:

1. **Fail Early**: Errors should be caught at the earliest possible phase. Build-time detection is preferred over schedule-time, which is preferred over runtime.
2. **Fail Clearly**: Error messages must be actionable. Instead of "Invalid Input," GEF aims for "Module 'Blur' expects Input 'image' of type Tensor, but received Scalar from 'ReadImage'."
3. **Fail Safely**: No error should ever leave the system in a corrupted state. `ResourcePool` management ensures that even when a module fails, resources are either correctly released or kept in a known state for inspection.
