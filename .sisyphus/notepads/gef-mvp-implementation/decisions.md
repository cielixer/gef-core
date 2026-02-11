### Architecture Decisions
- Replaced C++20 modules with traditional headers + dlopen-based plugin system for MVP to avoid toolchain instability and simplify build process.
- Hardcoded module loading in MVP tests to skip YAML parsing complexity.
- Context API uses proxy types to prevent accidental copies while maintaining a simple interface for researchers.
