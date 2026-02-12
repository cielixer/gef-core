#ifndef GEF_MACROS_H_
#define GEF_MACROS_H_

#include <gef/Common.h>
#include <gef/Context.h>

#define GEF_INPUT(type, name) {(name), GEF_ROLE_INPUT, #type}
#define GEF_OUTPUT(type, name) {(name), GEF_ROLE_OUTPUT, #type}
#define GEF_INOUT(type, name) {(name), GEF_ROLE_INOUT, #type}
#define GEF_CONFIG(type, name) {(name), GEF_ROLE_CONFIG, #type}

#define GEF__BINDING_SENTINEL {nullptr, static_cast<gef_role_t>(0), nullptr}

#define GEF_MODULE(name_, version_, execute_fn_, ...)                                              \
    static constexpr gef_binding_t gef__bindings[] = {                                             \
        __VA_OPT__(__VA_ARGS__, ) GEF__BINDING_SENTINEL};                                          \
    static constexpr size_t gef__num_bindings =                                                    \
        (sizeof(gef__bindings) / sizeof(gef__bindings[0])) - 1u;                                   \
    static const gef_metadata_t gef__metadata = {                                                  \
        .module_name  = (name_),                                                                   \
        .version      = (version_),                                                                \
        .bindings     = gef__bindings,                                                             \
        .num_bindings = gef__num_bindings,                                                         \
    };                                                                                             \
    extern "C" const gef_metadata_t* gef_get_metadata(void) { return &gef__metadata; }             \
    extern "C" void gef_execute(gef::Context& ctx) { execute_fn_(ctx); }

#endif
