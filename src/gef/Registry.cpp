#include <gef/Registry.h>
#include <stdexcept>

namespace gef {

Registry::Registry() = default;

Registry::~Registry() = default;

void Registry::registerModule(const gef_metadata_t* metadata, gef_execute_fn execute) {
    if (!metadata) {
        throw std::invalid_argument("Metadata cannot be null");
    }
    if (!metadata->module_name) {
        throw std::invalid_argument("Module name cannot be null");
    }
    if (!execute) {
        throw std::invalid_argument("Execute function cannot be null");
    }

    modules_[metadata->module_name] = ModuleEntry{metadata, execute};
}

const ModuleEntry* Registry::getModule(const char* name) {
    if (!name) {
        throw std::invalid_argument("Module name cannot be null");
    }

    auto it = modules_.find(name);
    if (it == modules_.end()) {
        return nullptr;
    }

    return &it->second;
}

} // namespace gef
