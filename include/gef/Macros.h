#ifndef GEF_MACROS_H_
#define GEF_MACROS_H_

#include <gef/Common.h>

#define GEF_MODULE(module_name, version) \
    static constexpr gef_binding_t _gef_bindings[] = {}; \
    static const gef_metadata_t _gef_metadata = { \
        .module_name = (module_name), \
        .version = (version), \
        .bindings = _gef_bindings, \
        .num_bindings = 0 \
    }; \
    extern "C" const gef_metadata_t* gef_get_metadata(void) { \
        return &_gef_metadata; \
    }

#define GEF_INPUT(type, name) \
    {(name), GEF_ROLE_INPUT, #type}

#define GEF_OUTPUT(type, name) \
    {(name), GEF_ROLE_OUTPUT, #type}

#define GEF_INOUT(type, name) \
    {(name), GEF_ROLE_INOUT, #type}

#define GEF_CONFIG(type, name) \
    {(name), GEF_ROLE_CONFIG, #type}

#endif
