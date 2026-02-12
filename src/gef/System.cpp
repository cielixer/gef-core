#include <gef/Module.h>
#include <gef/System.h>
#include <stdexcept>

namespace gef {

System::System()
    : loader_(std::make_unique<PluginLoader>()), registry_(std::make_unique<Registry>()) {}

System::~System() = default;

void System::loadModule(const char* path) {
    if (!path) {
        throw std::invalid_argument("Module path cannot be null");
    }

    void* handle = loader_->load(path);

    typedef const gef_metadata_t* (*GetMetadataFunc)(void);
    GetMetadataFunc get_metadata =
        reinterpret_cast<GetMetadataFunc>(loader_->getSymbol(handle, "gef_get_metadata"));

    const gef_metadata_t* metadata = get_metadata();
    if (!metadata) {
        loader_->unload(handle);
        throw std::runtime_error("Failed to retrieve module metadata");
    }

    auto execute = reinterpret_cast<gef_execute_fn>(loader_->getSymbol(handle, "gef_execute"));

    registry_->registerModule(metadata, execute);
}

void System::executeModule(const char* name, Context& ctx) {
    if (!name) {
        throw std::invalid_argument("Module name cannot be null");
    }

    const ModuleEntry* entry = registry_->getModule(name);
    if (!entry) {
        throw std::runtime_error(std::string("Module not found: ") + name);
    }

    entry->execute(ctx);
}

} // namespace gef
