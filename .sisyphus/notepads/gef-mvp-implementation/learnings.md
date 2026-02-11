### Patterns
- Prometheus plan format follows a strict structure: TL;DR, Context, Objectives, Verification, Execution, TODOs.
- MVP implementation focus on dynamic loading (dlopen) over C++20 modules for better compatibility and researcher DX.
- Query Function Pattern (extern "C") is preferred for metadata exposure.

### Conventions
- C++23 standard.
- Pixi for dependency management.
- Catch2 for testing.

## Module Implementation Pattern

### Bindings Array Declaration
The GEF macro system requires explicit bindings array declaration:
- Create a static `gef_binding_t` array with initializers using `GEF_INPUT()`, `GEF_OUTPUT()`, etc.
- The macro expansions create `gef_binding_t` structs with name, role, and type
- Set `num_bindings` to the exact count of bindings

### Module Class Structure
- Inherit from `gef::AtomicModule`
- Override `execute(gef::Context& ctx)`
- Use `ctx.input<T>(name)` and `ctx.output<T>(name)` which return reference-like objects
- Dereference with `*` or `->` to access the actual values

### CMake Integration
- `add_gef_module(name source)` creates a MODULE library
- Output location: `${CMAKE_BINARY_DIR}/modules/` 
- macOS: produces `.so` extension (set by GefModule.cmake)
- Symbol export: uses `extern "C"` to ensure `gef_get_metadata` is C-linkage compatible

### Example: Simple Addition Module
- Input bindings: `a` (int), `b` (int)
- Output binding: `sum` (int)
- Execute: reads a and b, writes their sum to output
- Clean compile, exports symbol correctly

## Final Cleanup (Task 9)

**Date:** Feb 11, 2026

### Files Removed
- `src/main.cpp` - Old prototype entry point referencing C++20 modules
- `src/core/config.h` - Legacy configuration header
- `src/core/config.cpp` - Legacy configuration implementation
- `src/modules/math.cppm` - Old C++20 module file
- `src/modules/utils.cppm` - Old C++20 module file

### Verification Results
✅ **No .cppm files remain:** `find src/ -name "*.cppm"` returns empty
✅ **Clean build succeeds:** `pixi run clean && pixi run build && pixi run test`
✅ **libgef.dylib exists:** 37K at `build/lib/libgef.dylib`
✅ **libexample_add.so exists:** 33K at `build/modules/libexample_add.so`
✅ **All tests pass:** 1/1 plugin_system_tests passed in 0.57s

### Final File Structure
```
src/
├── CMakeLists.txt
└── gef/
    ├── PluginLoader.cpp
    ├── Registry.cpp
    ├── Context.cpp
    └── System.cpp
```

Only production code remains. Pure C++23 standard headers with no legacy module references.
