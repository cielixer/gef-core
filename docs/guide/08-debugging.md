# Chapter 8: Debugging and Validation

In GEF, we want you to focus on your algorithms, not on chasing ghost-in-the-machine concurrency bugs. Because GEF is designed for high-performance research, it allows you to compose complex parallel systems with ease. However, with great parallelism comes the potential for complex dataflow errors.

GEF’s validation layer is designed to catch these errors as early as possible—often before a single line of your module code even executes.

## The Three Lines of Defense

Validation in GEF happens in three distinct phases. Think of these as filters that catch progressively more subtle bugs.

### 1. Build-time Validation (Static)
This happens the moment you load your YAML configuration or hot-reload a module. GEF checks the "contract" of your modules. It ensures that the inputs and outputs you've declared actually match up and that your graph is structurally sound. If your YAML has a typo or you've forgotten to wire a required input, you'll see an error here immediately.

### 2. Schedule-time Validation (DAG Analysis)
Once the structure is valid, the Static Scheduler builds an execution plan. During this phase, GEF analyzes which modules can run in parallel. It looks for "Parallel Groups" and verifies that no two modules in a group are trying to access the same data in an unsafe way (e.g., two modules trying to modify the same image simultaneously).

### 3. Runtime Validation (Debug-Only)
This is the final safety net. It only runs when you have **Debug Mode** enabled. It tracks actual memory access during execution to catch logical errors that only manifest when data is moving through the system, such as declaring a binding for modification but never actually writing to it.

---

## Common Error Scenarios

Let's look at the most frequent mistakes and how to fix them.

### Scenario 1: BindingConflict
A module cannot use the same name for an `Input` and an `Output`. GEF requires you to be explicit about whether you are reading data or producing new data.

**The Broken Code:**
```cpp
GEF_MODULE(BrightnessFilter) {
    GEF_INPUT(Image, frame);  // Read-only
    GEF_OUTPUT(Image, frame); // ERROR: Same name as input!
    
    void execute(Context& ctx) { ... }
}
```

**The Fix:**
Rename the output to something unique, or use `InOut` if you intend to modify the image in-place.
```cpp
GEF_MODULE(BrightnessFilter) {
    GEF_INPUT(Image, frame);
    GEF_OUTPUT(Image, brightened_frame); // Fixed: Unique name
}
```

### Scenario 2: UnresolvedBinding
This occurs when a module expects an input, but nothing in the graph provides it. This is usually a typo in the YAML or a missing upstream module.

**The Broken YAML:**
```yaml
type: flow
modules:
  - name: blur_task
    type: BlurModule
    # Expects input "image"
  - name: save_task
    type: SaveModule
    # Expects input "processed_image" but blur_task outputs "blurred_image"
```

**The Fix:**
Ensure names match or use explicit wiring to bridge the gap.
```yaml
type: flow
modules:
  - name: blur_task
    type: BlurModule
    # outputs: blurred_image
  - name: save_task
    type: SaveModule
    inputs:
      processed_image: blur_task.blurred_image # Fixed: Explicit wiring
```

### Scenario 3: TypeMismatch
GEF is strongly typed. If Module A outputs a `Scalar` but Module B expects a `Tensor`, the validation layer will stop execution at build-time.

**The Fix:**
Check the `GEF_INPUT` and `GEF_OUTPUT` macros in your C++ code. You may need to insert a conversion module (e.g., one that wraps a `Scalar` into a 1x1 `Tensor`) to make the data types compatible.

### Scenario 4: InOutRace
This is a concurrency error. It happens when two modules in a `Flow` are scheduled to run in parallel, and both try to modify the same resource using `InOut` bindings.

**The Scenario:**
```
       /--> [SharpenModule (InOut: image)] --\
[Read]                                        --> [Display]
       \--> [DenoiseModule (InOut: image)] --/
```
In a `Flow`, the modules in the middle branch are eligible for parallel execution. If both attempt to mutate `image` at the same time, the result is undefined.

**The Fix:**
Force a sequence by using a `Pipeline` instead of a `Flow`, or use explicit wiring to create a dependency:
```yaml
# In the Flow config
wiring:
  - from: SharpenModule.image
    to: DenoiseModule.image
```
This tells GEF that `DenoiseModule` must wait for `SharpenModule` to finish its mutation.

### Scenario 5: CyclicDependency
A graph must be a Directed Acyclic Graph (DAG). You cannot have a loop where A depends on B and B depends on A.

**The Scenario:**
```
[A] --> [B] --\
 ^            |
 \------------/
```

**The Fix:**
Redesign the logic to be linear or use "Feedback" patterns (which usually involve persistent data or a separate execution pass). GEF does not allow cycles within a single `Flow` execution.

---

## Debug Mode Features

When you enable Debug Mode, GEF provides additional tools to help you optimize your research code.

### InOut Write Detection (`InOutNoWrite`)
If you declare an `InOut` binding, GEF expects you to actually modify that data. If you only read from it, you should have used an `Input` binding instead. 
- **Why it matters**: `InOut` bindings require exclusive access, which can prevent other modules from running in parallel. `Input` bindings allow concurrent reads.
- **The Warning**: If GEF detects an `InOut` was never written to, it will issue a warning in Debug Mode. This is a hint to change it to `Input` to speed up your system.

### Debug-Only Profiling
Debug Mode enables built-in tracking for:
- **Module Execution Time**: How long each `execute()` call takes.
- **Memory Usage**: Tracking which modules are the "memory hogs" by monitoring the `Managed ResourcePool`.

### Zero Overhead in Release
We understand that every microsecond counts in production. All these safety checks and profiling hooks are compiled out in Release builds. You get the safety during development and the full speed of the hardware in production.

---

## How to Enable Debug Mode

Debug mode is controlled via a build configuration flag. If you are using `pixi`, you can typically toggle this in your environment settings or by passing a flag to the build command:

```bash
# Example build command with debug enabled
pixi run build --debug
```

In your C++ code, you can also check if validation is enabled using:
```cpp
#ifdef GEF_ENABLE_VALIDATION
    // Custom debug-only logic here
#endif
```

---

## Reading Validation Error Messages

GEF aims to provide "compiler-grade" error messages that tell you exactly what went wrong and where.

**Example Output:**
```text
[GEF VALIDATION ERROR] UnresolvedBinding in 'PreProcessFlow'
Module: 'NoiseFilter' (instance: 'denoise_01')
Error: Input 'alpha_mask' has no provider upstream.
Upstream providers: ['source_frame', 'config_params']
Suggested Fix: Verify the name 'alpha_mask' in your YAML wiring.
```

The error message includes:
1. **The Phase**: Where the error was caught (Build, Schedule, or Runtime).
2. **The Context**: Which Flow and which specific module instance is failing.
3. **The Data**: What exactly is missing or conflicting.
4. **The Hint**: A practical suggestion for how to fix it.

---

## Tips for Debugging Common Mistakes

- **Check Your Bindings First**: 90% of "weird" behavior is caused by using the wrong binding kind. Ask yourself: "Am I just reading this? Am I creating a new one? Or am I changing it in-place?"
- **Use ASCII Art for Complex Flows**: If you are getting `InOutRace` errors, try sketching your Flow. If two modules that use the same `InOut` resource aren't connected by an arrow, they are candidates for a race.
- **Trust the Warnings**: Don't ignore `UnusedOutput` or `InOutNoWrite` warnings. They are GEF's way of telling you that your system could be faster or simpler.
- **Reload is Your Friend**: Since GEF supports hot-reloading, you can fix a YAML error or a binding in your C++ code and see the validation result immediately without restarting the entire system.

---

## Cross-References

For a deep dive into the technical implementation of these checks, see the [Validation Layer Architecture](../design/validation-layer.md).

In the next chapter, we will explore how to integrate **External Libraries** like OpenCV and Eigen into your GEF modules to build even more powerful algorithms.

[Go to Chapter 09: External Libraries >](09-external-libraries.md)
