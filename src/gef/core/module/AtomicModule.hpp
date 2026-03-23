#ifndef GEF_CORE_MODULE_ATOMICMODULE_HPP_
#define GEF_CORE_MODULE_ATOMICMODULE_HPP_

#include "gef/core/module/AtomicModule.h"

#include <boost/dll/shared_library.hpp>

namespace gef {

struct AtomicModuleState {
    boost::dll::shared_library library;
    const gef_metadata_t*      metadata    = nullptr;
    gef_execute_fn_t           execute     = nullptr;
    bool                       owns_handle = false;
};

auto makeAtomicModuleState() -> AtomicModuleStatePtr;

auto createAtomicModule(void* handle, const gef_metadata_t* metadata,
                        gef_execute_fn_t execute) -> AtomicModule;

auto createAtomicModuleWithLibrary(boost::dll::shared_library library,
                                    const gef_metadata_t* metadata,
                                    gef_execute_fn_t execute) -> AtomicModule;

} // namespace gef

#endif // GEF_CORE_MODULE_ATOMICMODULE_HPP_
