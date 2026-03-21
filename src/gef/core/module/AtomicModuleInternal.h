#ifndef GEF_CORE_MODULE_ATOMICMODULE_INTERNAL_H_
#define GEF_CORE_MODULE_ATOMICMODULE_INTERNAL_H_

#include "gef/core/module/AtomicModule.h"

namespace gef {

auto createAtomicModule(void* handle, const gef_metadata_t* metadata,
                        gef_execute_fn_t execute) -> AtomicModule;

} // namespace gef

#endif // GEF_CORE_MODULE_ATOMICMODULE_INTERNAL_H_
