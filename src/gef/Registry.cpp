#include <gef/Registry.h>
#include <stdexcept>

namespace gef {

Registry::Registry() = default;

Registry::~Registry() = default;

void Registry::registerModule(const gef_metadata_t* metadata) {
    if (!metadata) {
        throw std::invalid_argument("Metadata cannot be null");
    }
    if (!metadata->module_name) {
        throw std::invalid_argument("Module name cannot be null");
    }
    
    modules_[metadata->module_name] = metadata;
}

const gef_metadata_t* Registry::getModule(const char* name) {
    if (!name) {
        throw std::invalid_argument("Module name cannot be null");
    }
    
    auto it = modules_.find(name);
    if (it == modules_.end()) {
        return nullptr;
    }
    
    return it->second;
}

}
