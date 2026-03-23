#include "AtomicModule.hpp"

#include "gef/core/binding/Context.h"

#include <utility>

namespace gef {

auto makeAtomicModuleState() -> AtomicModuleStatePtr {
    return AtomicModuleStatePtr(new AtomicModuleState{});
}

void AtomicModuleStateDeleter::operator()(AtomicModuleState* state) const {
    if (!state) {
        return;
    }

    if (state->owns_handle && state->library.is_loaded()) {
        state->library.unload();
    }

    delete state;
}

auto moduleMetadata(const AtomicModule& module) -> const gef_metadata_t* {
    return module.state ? module.state->metadata : nullptr;
}

void executeAtomicModule(const AtomicModule& module, Context& ctx) {
    if (module.state && module.state->execute) {
        module.state->execute(ctx);
    }
}

auto createAtomicModule(void* /*handle*/, const gef_metadata_t* metadata,
                        gef_execute_fn_t execute) -> AtomicModule {
    AtomicModule module;
    auto state = makeAtomicModuleState();
    state->metadata = metadata;
    state->execute = execute;
    state->owns_handle = true;
    module.state = std::move(state);
    return module;
}

auto createAtomicModuleWithLibrary(boost::dll::shared_library library,
                                    const gef_metadata_t* metadata,
                                    gef_execute_fn_t execute) -> AtomicModule {
    AtomicModule module;
    auto state = makeAtomicModuleState();
    state->library = std::move(library);
    state->metadata = metadata;
    state->execute = execute;
    state->owns_handle = true;
    module.state = std::move(state);
    return module;
}

auto cloneAtomicModuleNonOwning(const AtomicModule& module) -> AtomicModule {
    if (!module.state) {
        return AtomicModule{};
    }
    AtomicModule clone;
    clone.state = makeAtomicModuleState();
    clone.state->metadata = module.state->metadata;
    clone.state->execute = module.state->execute;
    clone.state->owns_handle = false;
    return clone;
}

} // namespace gef
