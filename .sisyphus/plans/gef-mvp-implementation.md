# GEF MVP Implementation — Core Plugin System

## TL;DR

> **Quick Summary**: Implement the Minimum Viable Product (MVP) of GEF, focusing on the C++23 plugin architecture. This phase replaces the experimental C++20 modules approach with a robust `dlopen`-based system where Atomic Modules are compiled as shared libraries. The goal is to prove that a module can be defined with macros, compiled independently, loaded dynamically, and executed via a type-safe Context API.
> 
> **Deliverables**:
> - Updated `pixi.toml` and `CMakeLists.txt` for a standard C++23/CMake/Ninja/Catch2 environment.
> - Core GEF headers (`Module.h`, `Context.h`, `Macros.h`) in `include/gef/`.
> - Core GEF implementation (Plugin loader, Module Registry) in `src/gef/`.
> - A CMake helper for easy module creation.
> - One working example module in `modules/`.
> - Automated Catch2 tests verifying the full "load-and-execute" lifecycle.
> 
> **Estimated Effort**: Medium
> **Parallel Execution**: YES — 3 waves
> **Critical Path**: Task 1 & 2 (Build System) → Task 3 (Headers) → Task 4 & 5 (Core & Helpers) → Task 6 & 7 (Example & Test)

---

## Context

### Original Request
The user wants to transition from a prototype using C++20 modules (which proved problematic) to the intended architecture: a plugin-based system using C++23. The focus is strictly on the **Module** and **Dynamic Linking** aspects. System, Scheduler, and ResourcePool components should be kept as simple as possible (stubs) to support the module loading demonstration. The environment must be self-contained using **pixi**.

### MVP Goals
- **Researcher DX**: Define a module with macros, run a single build command, and have it loaded.
- **Dynamic Loading**: Use `dlopen` to load `.so`/`.dylib` files at runtime.
- **Metadata**: Use the "Query Function Pattern" (`extern "C" gef_get_metadata`) for discovery.
- **Context API**: Access inputs/outputs/configs through a type-safe `Context` object.
- **Modern C++**: Use C++23 features (like `std::expected`, `std::print`, etc. where appropriate).

---

## Work Objectives

### Core Objective
Build the foundation of GEF that allows researchers to write, compile, and run Atomic Modules as dynamic plugins.

### Concrete Deliverables
- `pixi.toml`: Standardized environment with CMake, Ninja, Catch2.
- `CMakeLists.txt`: Root and `src/` files configured for `libgef` (shared) and plugins.
- `include/gef/`: All public headers for the framework.
- `src/gef/`: Implementation of plugin loading and registry.
- `modules/example_add.cpp`: A simple "add" module demonstrating the API.
- `tests/test_plugin_system.cpp`: Catch2 test suite.
- `CMakePresets.json`: Developer-friendly build presets.

### Definition of Done
- [ ] `pixi run test` passes all tests.
- [ ] `libgef.so` (or `.dylib`) is built and contains the registry/loader logic.
- [ ] `modules/example_add.so` is built and exports `gef_get_metadata`.
- [ ] Test successfully loads `example_add.so`, inspects its metadata, and executes it.
- [ ] No C++20 `.cppm` files remain in the project (clean sweep).
- [ ] Zero compiler warnings in the core and example module.

### Must Have
- **C++23** toolchain.
- **dlopen/dlsym** for dynamic loading.
- **Macros**: `GEF_MODULE`, `GEF_INPUT`, `GEF_OUTPUT`, `GEF_CONFIG`.
- **Context API**: `ctx.input<T>(name)`, `ctx.output<T>(name)`, `ctx.config<T>(name)`.
- **Query Function**: `extern "C" const gef_metadata_t* gef_get_metadata()`.
- **Catch2** for all verifications.
- **Pixi** as the sole entry point for environment setup.

### Must NOT Have (Guardrails)
- **NO C++20 Modules**: Remove all `.cppm` and `import` statements.
- **NO YAML Parsing**: Hardcode the module loading in the test for now.
- **NO Complex Scheduler**: The "System" just calls `module->execute(ctx)` directly.
- **NO ResourcePool Internals**: Use simple `std::any` or raw pointers in the stub ResourcePool.
- **NO Parallel Execution**: Everything runs sequentially in the MVP.
- **NO External Dependencies**: Except for standard ones (Catch2, etc. managed by pixi).

---

## Verification Strategy

> **UNIVERSAL RULE: ZERO HUMAN INTERVENTION**
> All verification via automated checks.

### Test Decision
- **Infrastructure**: Pixi task `test` runs `ctest`.
- **Automated tests**: Catch2 unit tests in `tests/`.
- **Framework**: Catch2 v3.

### Agent-Executed QA Scenarios (MANDATORY — ALL tasks)

Every task includes verification via shell commands checking file existence, build success, and test passes.

| Type | Tool | How Agent Verifies |
|------|------|-------------------|
| **Build System** | `pixi run build` | Verify zero errors and expected artifact existence |
| **Code Structure** | `ls`, `grep` | Verify headers are in `include/gef/` and macros are present |
| **Plugin Export** | `nm -D` or `objdump` | Verify `gef_get_metadata` is exported from the shared library |
| **Functionality** | `pixi run test` | Verify logic (e.g., 2+2=4 in the example module) |

---

## Execution Strategy

### Parallel Execution Waves

```
Wave 1 (Foundation):
+-- Task 1: Rewrite pixi.toml (Environment)
+-- Task 2: Rewrite CMakeLists.txt (Build Rules)

Wave 2 (Core Development):
+-- Task 3: Create Core Headers (The Interface)
+-- Task 4: Implement Plugin Loader & Registry (The Engine)
+-- Task 5: CMake Module Helper (The DX)

Wave 3 (Integration & Verification):
+-- Task 6: Create Example Module
+-- Task 7: Implement Catch2 Tests
+-- Task 8: Add CMakePresets.json
+-- Task 9: Final Cleanup & Verification
```

### Dependency Matrix

| Task | Depends On | Blocks | Can Parallelize With |
|------|------------|--------|---------------------|
| 1 (Pixi) | None | 2, 4, 7 | 3 |
| 2 (CMake) | 1 | 4, 5, 6 | 3 |
| 3 (Headers) | None | 4, 6, 7 | 1, 2 |
| 4 (Core) | 1, 2, 3 | 7, 9 | 5 |
| 5 (Helper) | 2 | 6 | 4 |
| 6 (Example) | 3, 5 | 7 | None |
| 7 (Tests) | 1, 3, 4, 6 | 9 | 8 |
| 8 (Presets) | 2 | 9 | 7 |
| 9 (Final) | 7, 8 | None | None |

### Agent Dispatch Summary

| Wave | Tasks | Recommended Agents |
|------|-------|-------------------|
| 1 | 1, 2 | task(category="quick") |
| 2 | 3, 4, 5 | task(category="quick", load_skills=["git-master"]) |
| 3 | 6, 7, 8, 9 | task(category="quick", load_skills=["git-master"]) |

---

## TODOs

- [x] 1. Rewrite `pixi.toml` for standard C++23 environment

  **What to do**:
  - Update `pixi.toml` to remove `pixi-build-cmake` preview mode (unless strictly needed by user, but user asked for "standard").
  - Add standard dependencies: `cmake`, `ninja`, `cxx-compiler`, `catch2`.
  - Add tasks: `build`, `test`, `clean`.
  - Ensure C++23 support is guaranteed by the compiler version.

  **Must NOT do**:
  - Keep C++20 module specific dependencies.

  **Acceptance Criteria**:
  - `pixi.toml` exists and contains `catch2` and `cmake`.
  - `pixi run build` (even if it fails later) attempts to run cmake.

  **Agent-Executed QA Scenarios**:
  ```bash
  Scenario: Pixi environment is ready
    Tool: Bash
    Steps:
      1. pixi run --version
      2. grep "catch2" pixi.toml
  ```

  **Commit**: YES
  - Message: `build: rewrite pixi.toml for standard C++23 and Catch2`

---

- [x] 2. Rewrite root `CMakeLists.txt` for `libgef` core

  **What to do**:
  - Set `CMAKE_CXX_STANDARD 23`.
  - Remove C++20 modules experimental flags.
  - Define `libgef` as a `SHARED` library.
  - Set up include directories for `include/` and `src/`.
  - Ensure `libgef` exports symbols correctly (use `CMAKE_CXX_VISIBILITY_PRESET hidden` and `VISIBILITY_INLINES_HIDDEN`).

  **Must NOT do**:
  - Use `.cppm` files.

  **Acceptance Criteria**:
  - `CMakeLists.txt` no longer contains `import` or `CMAKE_EXPERIMENTAL_CXX_MODULE`.
  - `cmake -B build -G Ninja` succeeds.

  **Agent-Executed QA Scenarios**:
  ```bash
  Scenario: CMake configuration succeeds
    Tool: Bash
    Steps:
      1. pixi run build (or cmake -B build)
      2. test -f build/CMakeCache.txt
  ```

  **Commit**: YES
  - Message: `build: modernize CMakeLists.txt for C++23 and libgef shared library`

---

- [x] 3. Create Core Headers in `include/gef/`

  **What to do**:
  - Create `include/gef/Common.h`: Base types, roles (`GEF_ROLE_INPUT`, etc.), metadata structs.
  - Create `include/gef/Context.h`: The `Context` class interface with template methods `input<T>`, `output<T>`, etc.
  - Create `include/gef/Module.h`: `AtomicModule` base class and `gef_get_metadata` signature.
  - Create `include/gef/Macros.h`: `GEF_MODULE`, `GEF_INPUT`, etc., that generate the metadata struct and query function.

  **Must NOT do**:
  - Implement logic here; only declarations and inline/template code.

  **Acceptance Criteria**:
  - Files exist in `include/gef/`.
  - `Macros.h` contains `extern "C" const gef_metadata_t* gef_get_metadata()`.

  **Commit**: YES
  - Message: `feat: add core GEF headers for module and context API`

---

- [x] 4. Implement Plugin Loader and Registry in `src/gef/`

  **What to do**:
  - Create `src/gef/PluginLoader.cpp`: Wrapper for `dlopen`/`dlsym`.
  - Create `src/gef/Registry.cpp`: A simple map that stores `gef_metadata_t` by module name.
  - Create `src/gef/Context.cpp`: Minimal implementation of the Context that can hold and return data.
  - Create `src/gef/System.cpp`: A stub that coordinates loading a library and registering it.

  **Must NOT do**:
  - Implement a full scheduler or resource pool. Keep it to "store a pointer and call it".

  **Acceptance Criteria**:
  - `libgef.so` (or `.dylib`) builds successfully.

  **Commit**: YES
  - Message: `feat: implement plugin loader and module registry`

---

- [x] 5. Create CMake Module Helper (`cmake/GefModule.cmake`)

  **What to do**:
  - Create a CMake function `add_gef_module(NAME SOURCE)` that:
    - Creates a `MODULE` (shared) library.
    - Links against `libgef`.
    - Sets the output directory to `${CMAKE_BINARY_DIR}/modules`.
    - Handles platform-specific flags (e.g., `-fPIC`).

  **Acceptance Criteria**:
  - `cmake/GefModule.cmake` exists and is included in the main `CMakeLists.txt`.

  **Commit**: YES
  - Message: `build: add CMake helper for creating GEF modules`

---

- [x] 6. Create Example Module `modules/example_add.cpp`

  **What to do**:
  - Implement a module that:
    - Declares two inputs: `a` and `b` (integers).
    - Declares one output: `sum` (integer).
    - In `execute`, reads `a` and `b`, and writes `a + b` to `sum`.
  - Use the `GEF_MODULE` and `GEF_INPUT/OUTPUT` macros.

  **Acceptance Criteria**:
  - `modules/example_add.so` is built.
  - `nm -D modules/example_add.so | grep gef_get_metadata` shows the exported symbol.

  **Commit**: YES
  - Message: `feat: add example_add module as a plugin`

---

- [x] 7. Implement Catch2 Tests in `tests/`

  **What to do**:
  - Create `tests/test_plugin_system.cpp`.
  - Test Case 1: Load `example_add.so` and verify metadata (name, binding count).
  - Test Case 2: Instantiate module, provide input values, execute, and verify output value.
  - Test Case 3: Verify error handling for missing symbols or incompatible versions (stubs okay).

  **Acceptance Criteria**:
  - `pixi run test` passes.

  **Commit**: YES
  - Message: `test: add plugin loading and execution tests with Catch2`

---

- [x] 8. Add `CMakePresets.json`

  **What to do**:
  - Define `dev` preset with `Debug` build type and `Ninja` generator.
  - Define `release` preset.
  - Define a test preset that runs `ctest`.

  **Acceptance Criteria**:
  - `cmake --list-presets` shows the new presets.

  **Commit**: YES
  - Message: `build: add CMakePresets.json for easier developer workflow`

---

- [x] 9. Final Cleanup and Verification

  **What to do**:
  - Remove all old `src/*.cppm`, `src/core/*.cpp`, `src/main.cpp` (if no longer needed).
  - Ensure `docs/` are unaffected.
  - Run a final full build and test cycle from a clean state.

  **Acceptance Criteria**:
  - No C++20 module artifacts remain.
  - Build/Test passes.

  **Commit**: YES
  - Message: `chore: final cleanup of legacy C++20 module artifacts`
