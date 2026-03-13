# GEF — General Engine Framework

**Generated:** 2026-02-22  
**Commit:** f05e429  
**Branch:** main

## OVERVIEW

C++23 shared library for dynamic module loading via dlopen/dlsym with C ABI boundary. Modules are self-contained `.so` files exporting `gef_get_metadata()` and `gef_execute()`. Compose into pipelines/flows, hot-reload at runtime.

## STRUCTURE

```
gef/
├── include/gef/       # Public headers — the entire API surface
│   ├── Common.h       # C ABI types (gef_metadata_t, gef_binding_t, gef_role_t)
│   ├── Context.h      # Header-only: type-erased binding container (std::any)
│   ├── Error.h        # ErrorCode enum + Error struct for std::expected
│   ├── Flow.h         # DAG composition of module instances
│   ├── Macros.h       # GEF_MODULE macro — generates extern "C" symbols
│   ├── Module.h       # C ABI function declarations (for reference only)
│   ├── ModuleRegistry.h  # Module storage + atomic loader free functions
│   ├── ModuleVariant.h   # AtomicModule, FlowModule, PipelineModule, ModuleDef
│   ├── Scheduler.h    # Topological sort (Kahn's, lexicographic tie-break)
│   └── System.h       # Facade: load + execute modules
├── src/gef/           # Implementations (4 TUs)
│   ├── ModuleRegistry.cpp  # loadAtomicModule, registry CRUD, hot-reload logic
│   ├── System.cpp          # loadModule orchestration, executeModule dispatch
│   ├── Scheduler.cpp       # Kahn's algorithm
│   └── Flow.cpp            # DAG execution, data routing, config injection
├── modules/           # Example atomic modules (each → .so via add_gef_module)
├── tests/             # Catch2 tests (test_module_system, test_flow)
├── cmake/             # GefModule.cmake (add_gef_module helper), config template
├── docs/              # DO NOT TOUCH — design docs + tutorial guides
├── pixi.toml          # Deps + tasks (configure, build, test, clean)
├── Taskfile.yml       # Alternative task runner (install, build, test, format)
└── CMakePresets.json  # dev (Debug) + release presets
```

## WHERE TO LOOK

| Task | Location | Notes |
|------|----------|-------|
| Add new public type | `include/gef/` | Follow existing `#ifndef GEF_*_H_` guard pattern |
| Add new implementation | `src/gef/` + `src/CMakeLists.txt` | Must add to `target_sources(gef PRIVATE ...)` |
| Create example module | `modules/` | Auto-discovered by `file(GLOB)` in modules/CMakeLists.txt |
| Write tests | `tests/` | Link `gef` + `Catch2::Catch2WithMain`, add to `tests/CMakeLists.txt` |
| Module loading flow | `System.cpp` → `ModuleRegistry.cpp` | `loadModule → loadAtomicModule → dlopen → dlsym` |
| Hot-reload logic | `ModuleRegistry.cpp:85-178` | Path normalization + unload-by-name + re-insert |
| C ABI contract | `Common.h` + `Macros.h` + `Module.h` | What .so files must export |
| DAG execution | `Flow.cpp` + `Scheduler.cpp` | Topo-sort → per-instance context → execute → copy outputs |
| Build configuration | `CMakeLists.txt` (root) | C++23, shared lib, spdlog, install targets |
| Module build helper | `cmake/GefModule.cmake` | `add_gef_module(NAME SOURCE)` — MODULE lib + .so suffix |

## CODE MAP

### Data Flow: Module Load → Execute

```
.so file
  ↓ dlopen + dlsym
AtomicModule { handle, metadata*, execute_fn }
  ↓ stored in
ModuleRegistry.atomic_modules_  (hot-reload cache, keyed by name)
  ↓ wrapped into
ModuleDef { name, ModuleSignature, ModuleVariant }
  ↓ stored in
ModuleRegistry.defs_  (keyed by ModuleId)
  ↓ dispatched via
std::visit(overloaded{...}, def.variant)  →  execute(Context&)
```

### Key Types

| Symbol | Location | Role |
|--------|----------|------|
| `gef_metadata_t` | Common.h | C ABI: module name, version, bindings array |
| `gef_binding_t` | Common.h | C ABI: name + role + type_name |
| `GEF_MODULE(ver, fn, ...)` | Macros.h | Generates `gef_get_metadata` + `gef_execute` |
| `AtomicModule` | ModuleVariant.h:47 | `{handle, metadata*, execute}` — raw .so handle |
| `ModuleDef` | ModuleVariant.h:64 | `{name, signature, variant}` — registry entry |
| `ModuleVariant` | ModuleVariant.h:62 | `variant<Atomic, Flow, Pipeline>` |
| `ModuleSignature` | ModuleVariant.h:28 | C++ metadata: version + bindings |
| `ModuleId` | ModuleVariant.h:13 | `uint32_t` index into `defs_` vector |
| `Context` | Context.h | Type-erased binding store (`map<string, any>`) |
| `ModuleRegistry` | ModuleRegistry.h | Module storage + atomic module cache |
| `System` | System.h | Facade: owns `ModuleRegistry`, load + execute |
| `Flow` | Flow.h | DAG: instances + typed edges + config + execution |
| `Scheduler` | Scheduler.h | Static `topologicalSort(nodes, edges)` |
| `Error` / `ErrorCode` | Error.h | `{code, message}` for `std::expected` |

### ModuleRegistry Private State

```
defs_                  vector<ModuleDef>          — ModuleId → ModuleDef
name_to_id_            map<string, ModuleId>      — qualified name → ID
atomic_modules_        map<string, AtomicModule>   — hot-reload cache
atomic_name_to_path_   map<string, string>         — module_name → normalized path
atomic_path_to_name_   map<string, string>         — normalized path → module_name
```

## CONVENTIONS

### Style (enforced by .clang-format)
- LLVM base, 4-space indent, 100-column limit
- Left-aligned pointers (`int* p`), sorted includes
- `// clang-format off` around `GEF_MODULE(...)` macro invocations

### Naming
- **Classes/Structs**: `PascalCase` (`ModuleRegistry`, `AtomicModule`)
- **Functions/Methods**: `camelCase` (`loadModule`, `topologicalSort`)
- **Variables/Members**: `snake_case` with trailing `_` for privates (`defs_`, `name_to_id_`)
- **Enums/Constants**: `PascalCase` for C++ (`ErrorCode::FileNotFound`), `UPPER_SNAKE` for C ABI (`GEF_ROLE_INPUT`)
- **Include guards**: `#ifndef GEF_FILENAME_H_` (not `#pragma once`)

### C++ Style
- C++23 required: `std::expected`, `[[nodiscard]]`, `[[unlikely]]`, `contains()`, designated initializers
- Trailing return types: `auto foo() -> ReturnType`
- Error handling: `std::expected<T, Error>` for recoverable, `throw` for programmer errors (empty name, duplicates)
- No inheritance for modules — use `std::variant` + `overloaded` visitor pattern
- `friend` free functions for registry access (⚠️ pending refactor to remove)

### Module Authoring Pattern
```cpp
#include <gef/Macros.h>
void execute(gef::Context& ctx) { /* use ctx.input/output/inout/config */ }
// clang-format off
GEF_MODULE("0.1.0", execute, GEF_INPUT(int, "x"), GEF_OUTPUT(int, "y"))
// clang-format on
```

## ANTI-PATTERNS (THIS PROJECT)

- **No `class` inheritance for modules** — user explicitly hates OOP; uses variant dispatch
- **No `as any` / `@ts-ignore` equivalents** — don't suppress type errors
- **No touching `docs/`** — design docs and guides are off-limits
- **No complex features** — focus only on modules and dynamic linking
- **No `friend` declarations** — pending removal; don't add new ones
- **Don't install anything except via pixi** — zero system deps beyond pixi itself

## COMMANDS

```bash
# Full build + test cycle (from zero)
pixi install
pixi run build          # configure + build (Release, Ninja)
pixi run test           # ctest --output-on-failure

# Individual steps
pixi run configure      # cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
pixi run clean          # rm -rf build

# Alternative: Taskfile
task install && task build && task test
task format             # clang-format all C++ files
task rebuild            # clean + install + build
```

### Test Targets
- `module_system_tests` — ModuleRegistry, loadAtomicModule, System load/execute, Context, hot-reload
- `flow_tests` — Scheduler topo-sort, Flow addModule/connect/execute, DAG patterns

## NOTES

- **macOS .so convention**: `GefModule.cmake` forces `.so` suffix even on macOS (not `.dylib`) for cross-platform dlopen consistency
- **GEF_MODULE_NAME**: Injected at compile time via `target_compile_definitions` — matches CMake target name, which matches filename stem
- **Hot-reload**: Same path → unload old, load new. Same name from different path → unload old name. Destructor dlcloses all handles.
- **Context stores `std::any` wrapping `T*`**: Bindings are pointer-based. `ctx.set_binding("x", std::any(&value))` then `ctx.input<T>("x")` dereferences.
- **No CI**: No GitHub Actions or similar. Build/test is local only.
- **Pending refactor**: `loadAtomicModule` currently takes `ModuleRegistry&` and uses `friend` — planned to become pure `(path) → expected<AtomicModule, Error>` with separate registry registration.
- **test_main.cpp**: Legacy sanity check with `int main()` — not a Catch2 test, not registered with CTest. Vestigial.
