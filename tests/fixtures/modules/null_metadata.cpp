#include <gef/core/binding/Common.h>
#include <gef/core/binding/Context.h>

extern "C" {
const gef_metadata_t* gef_get_metadata() {
    return nullptr;
}

void gef_execute(gef::Context& /*ctx*/) {}
}
