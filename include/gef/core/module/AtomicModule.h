#ifndef GEF_CORE_MODULE_ATOMICMODULE_H_
#define GEF_CORE_MODULE_ATOMICMODULE_H_

#include "gef/core/binding/Common.h"

#include <memory>

namespace gef {

class Context;

using gef_execute_fn_t = void (*)(Context&);

struct AtomicModuleState;

struct AtomicModule {
    AtomicModule();
    ~AtomicModule();

    AtomicModule(AtomicModule&& other) noexcept;
    auto operator=(AtomicModule&& other) noexcept -> AtomicModule&;

    AtomicModule(const AtomicModule&) = delete;
    auto operator=(const AtomicModule&) -> AtomicModule& = delete;

    auto _internalSetState(std::unique_ptr<AtomicModuleState> state) -> void;

private:
    std::unique_ptr<AtomicModuleState> state_;

    friend auto moduleMetadata(const AtomicModule& module) -> const gef_metadata_t*;
    friend void executeAtomicModule(const AtomicModule& module, Context& ctx);
    friend auto cloneAtomicModuleNonOwning(const AtomicModule& module) -> AtomicModule;
};

auto moduleMetadata(const AtomicModule& module) -> const gef_metadata_t*;
void executeAtomicModule(const AtomicModule& module, Context& ctx);

} // namespace gef

#endif // GEF_CORE_MODULE_ATOMICMODULE_H_
