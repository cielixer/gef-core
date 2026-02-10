# YAML Configuration Specification

## Overview

The YAML configuration format is the primary mechanism for defining the logical structure and execution parameters of a GEF (General Engine Framework) application. It serves as the bridge between independent, algorithm-focused Atomic Modules and the complex, hierarchical systems built by engineers.

GEF uses YAML to express the Directed Acyclic Graph (DAG) of computations, the wiring between module bindings, and the injection of static configuration parameters. This document specifies the schema, semantics, and resolution rules for GEF configuration files.

## Rationale: Why YAML?

During the design phase, several standard serialization formats were evaluated for expressing module composition and DAG structures:

| Format | Suitability for GEF | Rationale |
| :--- | :--- | :--- |
| **TOML** | Low | While excellent for flat configuration keys, TOML becomes syntactically heavy and unreadable when expressing deeply nested hierarchies or complex graph edges (wiring). |
| **JSON** | Medium | Universal and unambiguous, but lacks the human-readability and concise syntax required for a "researcher-first" developer experience. The lack of comments and verbose quoting makes it poor for large architecture definitions. |
| **YAML** | **High** | YAML strikes the ideal balance for DAG expression. It supports clean nested mappings, list-based module definitions, and multiple syntaxes for expressing edges (wiring). Its support for multi-line strings and comments is essential for documenting complex flows. |

While a custom Domain Specific Language (DSL) is planned for future iterations to further streamline the developer experience, YAML remains the authoritative configuration format for the current GEF architecture.

## Global Schema Structure

Every GEF configuration file defines a `module` object at the top level. Because GEF follows the Composite Pattern, a configuration file essentially defines a single Composite Module (Pipeline, Flow, or Batch) that may contain any number of child modules.

### Top-Level Fields

A module definition (whether top-level or nested) consists of the following primary fields:

*   **`module:`** (Object): The root container for the module definition.
*   **`name:`** (String): A unique identifier for this module instance within its parent's scope.
*   **`type:`** (String): The module type. Valid values: `pipeline`, `flow`, `batch`.
*   **`inputs:`** (List of Strings): Declaration of the input bindings this module exposes.
*   **`outputs:`** (List of Strings): Declaration of the output bindings this module produces.
*   **`configs:`** (List of Strings or Mappings): Declaration of the configuration parameters required by this module.
*   **`modules:`** (List of Objects): The list of child modules contained within this composite. **Note: The field name is strictly `modules:`, never `stages:`.**
*   **`wiring:`** (List of Strings/Objects): Explicit connection definitions between child modules or the interface boundary.
*   **`include:`** (String/List of Strings): Path(s) to external YAML files to be merged or referenced.

## Interface Boundaries: `inputs.` and `outputs.`

Composite modules (Pipelines, Flows, Batches) act as black boxes with their own defined interfaces. To wire child modules to the composite's own interface, GEF uses the reserved prefixes `inputs.` and `outputs.`.

*   **`inputs.[name]`**: References an input binding provided to the parent composite.
*   **`outputs.[name]`**: References an output binding that the parent composite must produce.

Internally, the framework treats the interface boundary as two invisible, "virtual" modules that act as the source and sink for the internal graph.

## Wiring Rules

Wiring defines the flow of data resources (identified by their binding names) between modules. GEF supports both implicit and explicit wiring.

### Implicit Wiring
By default, the GEF scheduler attempts to match module inputs to upstream outputs by name.
*   If Module B has an input named `image`, and an upstream Module A has an output named `image`, they are connected automatically.
*   In a **Pipeline**, this follows "variable list" semantics (see below).
*   In a **Flow**, name collisions result in a build-time error unless disambiguated.

### Explicit Wiring
Explicit wiring overrides or supplements implicit matching. GEF provides two syntaxes:

1.  **Arrow Syntax (String)**:
    `source_module.binding_name -> destination_module.binding_name`
    Example: `blur_filter.blurred_image -> edge_detector.image`

2.  **Structured Format (Mapping)**:
    ```yaml
    - from: blur_filter.blurred_image
      to: edge_detector.image
    ```

## Module-Specific Semantics

### Pipeline (`type: pipeline`)
A Pipeline executes its `modules` in a strict sequential order.

*   **Sequential Ordering**: Modules are executed exactly in the order they appear in the `modules` list.
*   **Last-Writer-Wins**: If multiple modules in a pipeline produce an output with the same name, the most recent writer's data is what downstream modules receive. The validation layer issues a warning when shadowing occurs.
*   **Skip-Connections**: An output from the first module is available to all subsequent modules, even if the second or third modules do not use it.

### Flow (`type: flow`)
A Flow defines a Directed Acyclic Graph (DAG) of modules, allowing for parallel execution.

*   **Parallelism**: The scheduler analyzes the `wiring` (and implicit bindings) to find modules with no mutual dependencies and executes them concurrently.
*   **Collision Error**: If two modules in a Flow produce outputs with the same name, a `BindingConflict` error is raised at build-time. All dataflow must be unambiguous.
*   **Explicit Wiring Requirement**: For complex DAGs where name-matching is insufficient or ambiguous, explicit `wiring` is mandatory.

### Batch (`type: batch`)
A Batch module is a wrapper that runs an `inner` module in parallel over a collection.

*   **`inner:`** (Object): Defines the module (Atomic or Composite) to be executed for each element.
*   **`split:`** (Object): Specifies the partitioning strategy.
    *   `strategy`: e.g., `batch_tensor_dim`, `batch_sequence`.
*   **`gather:`** (Object): Specifies the recombination strategy (usually mirrors `split`).
*   **`mapping:`** (Object): Defines how the split elements map to the `inner` module's inputs and how the `inner` outputs map to the gatherer.

## Configuration Inheritance and Disambiguation

GEF uses a flat namespace for configuration injection.

### Config Injection
When a Composite Module declares a `configs:` list, it exposes those keys to the `System`. These values are then injected into any child modules that declare the same config names.

### Key Collision Disambiguation
If two child modules require the same config key name (e.g., `threshold`) but must receive different values, the engineer can use **qualified names** in the configuration:

```yaml
module:
  name: detection_system
  type: flow
  configs:
    - blur_filter.threshold: 5
    - edge_detector.threshold: 12
```

The framework automatically maps the qualified key to the specific module instance's local `threshold` binding.

## Multi-file Composition (`include:`)

The `include:` field allows for modular configuration management.

*   **Path Resolution**: Paths are resolved relative to the directory of the file containing the `include` statement.
*   **Merging Logic**: Included files are parsed and treated as module definitions. They can be referenced by name or used to define child modules.
*   **Reuse**: A common "library" of module configurations can be maintained and included across multiple system definitions.

## Reference Examples

### Example 1: Sequential Pipeline
A simple sequential process: Read, Blur, Threshold, Save.

```yaml
module:
  name: simple_image_pipeline
  type: pipeline
  inputs: [input_path]
  outputs: [output_status]
  configs: [kernel_size, threshold_value]
  modules:
    - name: reader
      type: ImageReader
      # implicit wiring: inputs.input_path matches reader's expected input
    - name: blur
      type: GaussianBlur
      # implicit wiring: reader.image -> blur.image
    - name: threshold
      type: AdaptiveThreshold
      # implicit wiring: blur.image -> threshold.image
    - name: writer
      type: ImageWriter
      # explicit wiring to parent interface
      wiring:
        - threshold.image -> outputs.processed_image
```

### Example 2: Parallel Flow
A DAG where two filters run in parallel and their results are merged.

```yaml
module:
  name: parallel_enhancement_flow
  type: flow
  inputs: [image]
  outputs: [final_result]
  modules:
    - name: denoise
      type: DenoiseFilter
    - name: sharpen
      type: SharpenFilter
    - name: merger
      type: ImageMerger
  wiring:
    # Wire parent inputs to children
    - inputs.image -> denoise.image
    - inputs.image -> sharpen.image
    
    # Wire children to merger
    - denoise.output -> merger.layer_a
    - sharpen.output -> merger.layer_b
    
    # Wire merger to parent output
    - merger.result -> outputs.final_result
```

### Example 3: Batch Video Processor
Processing every frame of a video using a batch wrapper.

```yaml
module:
  name: video_batch_processor
  type: batch
  inputs: [video_tensor]
  outputs: [processed_video]
  split:
    strategy: batch_tensor_dim
    dim: 0 # Frame dimension
  gather:
    strategy: batch_tensor_dim
    dim: 0
  inner:
    type: pipeline
    modules:
      - name: gray
        type: GrayscaleFilter
      - name: contrast
        type: ContrastAdjust
  mapping:
    inputs:
      video_tensor: [gray.image]
    outputs:
      processed_video: [contrast.image]
```

### Example 4: Complex Wiring Syntax
Demonstrating arrow syntax and interface boundaries in a Flow.

```yaml
module:
  name: complex_wiring_demo
  type: flow
  inputs: [raw_data, mask]
  outputs: [summary_report]
  modules:
    - name: preprocessor
      type: DataCleaner
    - name: analyzer
      type: FeatureExtractor
    - name: reporter
      type: PDFGenerator
  wiring:
    - inputs.raw_data -> preprocessor.data
    - inputs.mask -> preprocessor.mask
    - preprocessor.cleaned -> analyzer.input
    - analyzer.features -> reporter.data
    - reporter.pdf -> outputs.summary_report
```

### Example 5: Multi-file Include
Organizing a system into multiple files.

```yaml
# main_system.yaml
module:
  name: production_system
  type: flow
  include: "submodules/preprocessing.yaml"
  modules:
    - name: prep
      type: preprocessing_module # Defined in the included file
    - name: main_logic
      type: MyAlgorithm
  wiring:
    - prep.output -> main_logic.input
```

## Summary of YAML Validation Rules

1.  **Schema Validity**: All mandatory fields (`name`, `type`) must be present for every module.
2.  **Terminology Enforcement**: The field `stages:` is strictly forbidden; `modules:` must be used for child lists.
3.  **Cycle Detection**: The `wiring` must not result in a directed cycle.
4.  **Interface Consistency**: A composite module must produce all bindings declared in its `outputs:` list.
5.  **Type Safety**: The data types of wired bindings must be compatible (e.g., `Image` to `Image`).
6.  **Binding Kind Constraints**: `InOut` bindings cannot be wired to multiple parallel writers.
