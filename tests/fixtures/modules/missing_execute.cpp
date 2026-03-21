#include <gef/core/binding/Common.h>

extern "C" {
const gef_metadata_t* gef_get_metadata() {
    static gef_metadata_t meta = {
        .module_name = "missing_execute",
        .version = "0.1.0",
        .bindings = nullptr,
        .num_bindings = 0
    };
    return &meta;
}
}
