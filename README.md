# GEF — General Engine Framework

A modular C++ framework that lets researchers write self-contained computation
modules and compose them into pipelines, parallel graphs, and batch processors —
then run immediately with hot-reload.

**Philosophy:** Modify one file, rebuild one shared library, run right away.

---

## Documentation

| Document | Audience | Description |
|----------|----------|-------------|
| [Tutorial](docs/guide/00-introduction.md) | Researchers | Hands-on guide from first module to full project |
| [Architecture Reference](docs/design/overview.md) | Engineers | Deep-dive into GEF internals and design rationale |

### Tutorial Chapters

00 · [Introduction](docs/guide/00-introduction.md)
01 · [Quick Start](docs/guide/01-quick-start.md)
02 · [Your First Module](docs/guide/02-your-first-module.md)
03 · [Bindings](docs/guide/03-bindings.md)
04 · [Composing Pipelines](docs/guide/04-composing-pipelines.md)
05 · [Flows & Parallelism](docs/guide/05-flows-and-parallelism.md)
06 · [Batch Processing](docs/guide/06-batch-processing.md)
07 · [Resource Management](docs/guide/07-resource-management.md)
08 · [Debugging & Profiling](docs/guide/08-debugging.md)
09 · [External Libraries](docs/guide/09-external-libraries.md)
10 · [Example Projects](docs/guide/10-example-projects.md)

---

## Prerequisites

- [pixi](https://pixi.sh) — package manager and environment
- C++23 compatible compiler

## Quick Start

```bash
pixi install
pixi run build
```

Then follow the [Quick Start guide](docs/guide/01-quick-start.md).

---

## License

TBD
