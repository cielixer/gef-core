#ifndef GEF_APP_H_
#define GEF_APP_H_

// Application builder facade: primary API for loading and executing GEF modules
//
// This header provides everything needed to build applications with GEF:
// - System for loading modules and managing the store
// - Flow for composing modules into DAGs (pipelines, parallel graphs)
// - Error handling for recoverable errors
// - ModuleStore for low-level store access if needed
//
// Example usage:
//   #include <gef/app.h>
//   gef::System system;
//   system.loadModule("path/to/module.so");
//   gef::Context ctx;
//   system.executeModule("module_name", ctx);

#include <gef/core/system/System.h>
#include <gef/core/system/Flow.h>
#include <gef/core/system/Error.h>
#include <gef/core/module/ModuleStore.h>

#endif // GEF_APP_H_
