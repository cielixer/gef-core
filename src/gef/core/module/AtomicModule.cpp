#include "gef/core/module/AtomicModule.h"

#include "gef/core/binding/Context.h"

#include <boost/dll/shared_library.hpp>
#include <utility>

namespace gef {

struct AtomicModuleState {
    boost::dll::shared_library library;
    const gef_metadata_t*      metadata    = nullptr;
    gef_execute_fn_t           execute     = nullptr;
    bool                       owns_handle = false;
};

AtomicModule::AtomicModule() : state_(std::make_unique<AtomicModuleState>()) {}

AtomicModule::~AtomicModule() {
    // Boost.DLL handles library unloading via RAII
    // Only unload if we own the handle
    if (state_ && state_->owns_handle && state_->library.is_loaded()) {
        state_->library.unload();
    }
}

AtomicModule::AtomicModule(AtomicModule&& other) noexcept = default;

auto AtomicModule::operator=(AtomicModule&& other) noexcept -> AtomicModule& = default;



auto moduleMetadata(const AtomicModule& module) -> const gef_metadata_t* {
    return module.state_ ? module.state_->metadata : nullptr;
}

void executeAtomicModule(const AtomicModule& module, Context& ctx) {
    if (module.state_ && module.state_->execute) {
        module.state_->execute(ctx);
    }
}

auto createAtomicModule(void* /*handle*/, const gef_metadata_t* metadata,
                        gef_execute_fn_t execute) -> AtomicModule {
    AtomicModule module;
    module.state_ = std::make_unique<AtomicModuleState>();
    module.state_->metadata = metadata;
    module.state_->execute = execute;
    module.state_->owns_handle = true;
    return module;
}

auto createAtomicModuleWithLibrary(boost::dll::shared_library library,
                                    const gef_metadata_t* metadata,
                                    gef_execute_fn_t execute) -> AtomicModule {
    AtomicModule module;
    module.state_ = std::make_unique<AtomicModuleState>();
    module.state_->library = std::move(library);
    module.state_->metadata = metadata;
    module.state_->execute = execute;
    module.state_->owns_handle = true;
    return module;
}

auto cloneAtomicModuleNonOwning(const AtomicModule& module) -> AtomicModule {
    if (!module.state_) {
        return AtomicModule{};
    }
    AtomicModule clone;
    clone.state_ = std::make_unique<AtomicModuleState>();
    clone.state_->metadata = module.state_->metadata;
    clone.state_->execute = module.state_->execute;
    clone.state_->owns_handle = false;
    return clone;
}

} // namespace gef
