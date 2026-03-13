#ifndef GEF_MODULE_HPP_
#define GEF_MODULE_HPP_

// Module author facade: minimal includes for writing GEF module plugins
//
// This header provides everything needed to write a self-contained GEF module:
// - GEF_MODULE macro for declaring module metadata
// - Binding macros (GEF_INPUT, GEF_OUTPUT, etc.)
// - Context for accessing module inputs/outputs/config
// - C ABI types for metadata
//
// Example usage:
//   #include <gef/module.hpp>
//   void execute(gef::Context& ctx) { ... }
//   GEF_MODULE("0.1.0", execute, GEF_INPUT(int, "x"), GEF_OUTPUT(int, "y"))

#include <gef/core/module/Macros.h>
#include <gef/core/binding/Context.h>
#include <gef/core/binding/Common.h>

#endif // GEF_MODULE_HPP_
