# GEF — General Engine Framework

**Generated:** 2026-03-13  
**Commit:** 79a7a3a  
**Branch:** main

## OVERVIEW

C++23 shared library for dynamic module loading via dlopen/dlsym with C ABI boundary. Modules are self-contained `.so` files exporting `gef_get_metadata()` and `gef_execute()`. Compose into DAG flows, hot-reload at runtime.

## STRUCTURE

```
gef/
├── include/gef/                    # Public API surface
│   ├── gef.hpp                     # Umbrella header (includes everything)
│   ├── module.hpp                  # Module author facade (Macros + Context + Common)
│   ├── app.h                       # App builder facade (System + Flow + Error + Store)
│   └── core/
│       ├── binding/
│       │   ├── Common.h            # C ABI types (gef_metadata_t, gef_binding_t, gef_role_t)
│       │   └── Context.h           # Header-only: type-erased binding container (std::any)
│       ├── module/
│       │   ├── Macros.h            # GEF_MODULE macro — generates extern "C" symbols
│       │   ├── Module.h            # C ABI function declarations (reference only)
│       │   ├── ModuleStore.h        # Module storage struct + free-function declarations
│       │   └── ModuleVariant.h     # AtomicModule (move-only RAII), CompositeModule, ModuleDef
│       ├── scheduler/
│       │   └── Scheduler.h         # Topological sort (Kahn's, lexicographic tie-break)
│       └── system/
│           ├── Error.h             # ErrorCode enum + Error struct for std::expected
│           ├── Flow.h              # DAG composition of module instances
│           └── System.h            # Facade: load + execute modules
├── src/gef/core/                   # Implementations (4 TUs, mirrors include hierarchy)
│   ├── module/ModuleStore.cpp      # loadAtomicModule (pure), store CRUD free functions
│   ├── scheduler/Scheduler.cpp     # Kahn's algorithm
│   └── system/
│       ├── System.cpp              # loadModule orchestration, executeModule dispatch
│       └── Flow.cpp                # DAG execution, data routing, config injection
├── modules/                        # Example atomic modules (each → .so via add_gef_module)
├── tests/                          # Catch2 tests (module_system_tests, flow_tests)
├── cmake/                          # GefModule.cmake helper + config template
├── docs/                           # DO NOT TOUCH — design docs + tutorial guides
├── pixi.toml                       # Deps + tasks (configure, build, test, clean)
├── Taskfile.yml                    # Alternative task runner (install, build, test, format)
└── CMakePresets.json               # dev (Debug) + release (Release) presets
```

## WHERE TO LOOK

| Task | Location | Notes |
|------|----------|-------|
| Add new public type | `include/gef/core/{domain}/` | Follow `#ifndef GEF_*_H_` guard pattern. Add to facade if public. |
| Add new implementation | `src/gef/core/{domain}/` + `src/CMakeLists.txt` | Must add to `target_sources(gef PRIVATE ...)` |
| Create example module | `modules/` | Auto-discovered by `file(GLOB)` in modules/CMakeLists.txt |
| Write tests | `tests/` | Link `gef` + `Catch2::Catch2WithMain`, add to `tests/CMakeLists.txt` |
| Module loading flow | `System.cpp` → `ModuleStore.cpp` | `loadModule → loadAtomicModule → dlopen → dlsym` |
| C ABI contract | `core/binding/Common.h` + `core/module/Macros.h` + `core/module/Module.h` | What .so files must export |
| DAG execution | `core/system/Flow.cpp` + `core/scheduler/Scheduler.cpp` | Topo-sort → per-instance context → execute → copy outputs |
| Build configuration | `CMakeLists.txt` (root) | C++23, shared lib, spdlog, install targets |
| Module build helper | `cmake/GefModule.cmake` | `add_gef_module(NAME SOURCE)` — MODULE lib + .so suffix |
| Facade headers | `include/gef/{gef.hpp,module.hpp,app.h}` | Role-specific API entry points |

## CODE MAP

### Facade Hierarchy

```
gef.hpp  (umbrella — includes both)
├── module.hpp  (module authors: Macros.h, Context.h, Common.h)
└── app.h       (app builders: System.h, Flow.h, Error.h, ModuleStore.h)
```

### Data Flow: Module Load → Execute

```
.so file
  ↓ dlopen + dlsym
AtomicModule { handle, metadata*, execute_fn }  (move-only RAII, dlclose on destroy)
  ↓ moved into
ModuleDef { name, ModuleSignature, ModuleVariant }
  ↓ stored in
ModuleStore.defs  (keyed by ModuleId)
  ↓ dispatched via
std::visit(overloaded{...}, def.variant)  →  execute(Context&)
```

### Data Flow: Flow (DAG) Execution

```
Flow.addModule("inst", "module_name")  →  instances_ map
Flow.connect<T>("inst1.out", "inst2.in")  →  edges_ with allocate/bind/copy lambdas
Flow.execute(ctx):
  1. Build DAG edges from connections
  2. Scheduler::topologicalSort(instances, edges)  →  execution order
  3. For each instance in order:
     a. Create local Context
     b. Bind inputs (from external ctx or data_store_)
     c. Bind outputs (to data_store_)
     d. Bind config values
     e. Execute module via store lookup + std::visit
  4. Copy outputs to external context
```

### Key Types

| Symbol | Location | Role |
|--------|----------|------|
| `gef_metadata_t` | core/binding/Common.h | C ABI: module name, version, bindings array |
| `gef_binding_t` | core/binding/Common.h | C ABI: name + role + type_name |
| `gef_role_t` | core/binding/Common.h | C ABI enum: INPUT/OUTPUT/INOUT/CONFIG |
| `Context` | core/binding/Context.h | Type-erased binding store (`map<string, any>`) |
| `GEF_MODULE(ver, fn, ...)` | core/module/Macros.h | Generates `gef_get_metadata` + `gef_execute` |
| `AtomicModule` | core/module/ModuleVariant.h:47 | `{handle, metadata*, execute}` — move-only RAII, dlclose on destroy |
| `ModuleDef` | core/module/ModuleVariant.h:64 | `{name, signature, variant}` — store entry |
| `ModuleVariant` | core/module/ModuleVariant.h:62 | `variant<AtomicModule, CompositeModule>` |
| `ModuleSignature` | core/module/ModuleVariant.h:28 | C++ metadata: version + bindings vector |
| `ModuleId` | core/module/ModuleVariant.h:13 | `uint32_t` index into `defs` vector |
| `ModuleStore` | core/module/ModuleStore.h | Module storage struct + free-function declarations |
| `System` | core/system/System.h | Facade: owns `ModuleStore`, load + execute |
| `Flow` | core/system/Flow.h | DAG: instances + typed edges + config + execution |
| `Scheduler` | core/scheduler/Scheduler.h | Static `topologicalSort(nodes, edges)` |
| `Error` / `ErrorCode` | core/system/Error.h | `{code, message}` for `std::expected` |
| `overloaded` | core/module/ModuleVariant.h:33 | Variadic visitor helper for `std::visit` |

### ModuleStore Public State

```
defs                   vector<ModuleDef>                    — ModuleId → ModuleDef
name_to_id             unordered_map<string, ModuleId>      — qualified name → ID
```

## CONVENTIONS

### Style (enforced by .clang-format)
- LLVM base, 4-space indent, 100-column limit
- Left-aligned pointers (`int* p`), sorted includes
- `// clang-format off` around `GEF_MODULE(...)` macro invocations

### Naming
- **Classes/Structs**: `PascalCase` (`ModuleStore`, `AtomicModule`)
- **Functions/Methods**: `camelCase` (`loadModule`, `topologicalSort`)
- **Variables/Members**: `snake_case` with trailing `_` for privates (`defs_`, `name_to_id_`)
- **Enums/Constants**: `PascalCase` for C++ (`ErrorCode::FileNotFound`), `UPPER_SNAKE` for C ABI (`GEF_ROLE_INPUT`)
- **Macros**: `UPPER_SNAKE` with `GEF_` prefix (`GEF_MODULE`, `GEF_INPUT`)
- **Include guards**: `#ifndef GEF_FILENAME_H_` (not `#pragma once`)

### C++ Style
- C++23 required: `std::expected`, `[[nodiscard]]`, `[[unlikely]]`, `contains()`, designated initializers
- Trailing return types: `auto foo() -> ReturnType`
- Error handling: `std::expected<T, Error>` for recoverable, `throw` for programmer errors (empty name, duplicates)
- No inheritance for modules — use `std::variant` + `overloaded` visitor pattern

### Module Authoring Pattern
```cpp
#include <gef/module.hpp>
void execute(gef::Context& ctx) {
    auto& x = ctx.input<int>("x");
    auto& y = ctx.output<int>("y");
    y = x * 2;
}
// clang-format off
GEF_MODULE("0.1.0", execute, GEF_INPUT(int, "x"), GEF_OUTPUT(int, "y"))
// clang-format on
```

### Header Layout (domain-based)
- `include/gef/core/binding/` — C ABI types + type-erased Context
- `include/gef/core/module/` — module variants, store, macros, ABI declarations
- `include/gef/core/scheduler/` — DAG scheduling utilities
- `include/gef/core/system/` — System facade, Flow composition, error types
- `src/gef/core/` mirrors this hierarchy exactly

## ANTI-PATTERNS (THIS PROJECT)

- **No `class` inheritance for modules** — variant dispatch only; no OOP
- **No type error suppression** — no `reinterpret_cast` except for dlsym results
- **No touching `docs/`** — design docs and guides are off-limits
- **No complex features** — focus only on modules and dynamic linking
- **No new `friend` declarations** — no `friend` usage in codebase
- **No deps outside pixi** — zero system deps beyond pixi itself
- **No `#pragma once`** — use `#ifndef GEF_*_H_` guards

## COMMANDS

```bash
# Full build + test cycle (from zero)
pixi install
pixi run build          # configure (Release, Ninja) + build
pixi run test           # ctest --output-on-failure

# Individual steps
pixi run configure      # cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
pixi run clean          # rm -rf build

# Alternative: Taskfile
task install && task build && task test
task format             # clang-format all C++ files in include/src/modules/tests
task rebuild            # clean + install + build

# CMake presets
cmake --preset dev      # Debug + Ninja + compile_commands.json
cmake --preset release  # Release + Ninja
```

### Test Targets
- `module_system_tests` — ModuleStore, loadAtomicModule, System load/execute, Context, hot-reload
- `flow_tests` — Scheduler topo-sort, Flow addModule/connect/execute, DAG patterns
- `test_facade_module` / `test_facade_app` — OBJECT libs verifying facade header compilability

## NOTES

- **macOS .so convention**: `GefModule.cmake` forces `.so` suffix even on macOS (not `.dylib`) for cross-platform dlopen consistency
- **GEF_MODULE_NAME**: Injected at compile time via `target_compile_definitions` — matches CMake target name, which matches filename stem
- **GEF_MODULE_DIR**: Injected into test executables pointing to `${CMAKE_BINARY_DIR}/modules`
- **Context stores `std::any` wrapping `T*`**: Bindings are pointer-based. `ctx.set_binding("x", std::any(&value))` then `ctx.input<T>("x")` dereferences.
- **No CI**: No GitHub Actions or similar. Build/test is local only.
- **test_main.cpp**: Legacy sanity check with `int main()` — not a Catch2 test, not registered with CTest. Vestigial.
- **Maps are `unordered_map`**: Store uses `std::unordered_map` (not `std::map`) for name lookups.
