#ifndef GEF_CORE_MODULE_ATOMICMODULE_H_
#define GEF_CORE_MODULE_ATOMICMODULE_H_

#include "gef/core/binding/Common.h"

#include <memory>

namespace gef {

struct Context;

using gef_execute_fn_t = void (*)(Context&);

struct AtomicModuleState;

struct AtomicModuleStateDeleter {
    void operator()(AtomicModuleState* state) const;
};

using AtomicModuleStatePtr = std::unique_ptr<AtomicModuleState, AtomicModuleStateDeleter>;

struct AtomicModule {
    AtomicModuleStatePtr state;
};

auto moduleMetadata(const AtomicModule& module) -> const gef_metadata_t*;
void executeAtomicModule(const AtomicModule& module, Context& ctx);
auto cloneAtomicModuleNonOwning(const AtomicModule& module) -> AtomicModule;

} // namespace gef

#endif // GEF_CORE_MODULE_ATOMICMODULE_H_
