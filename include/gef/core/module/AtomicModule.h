#ifndef GEF_ATOMICMODULE_H_
#define GEF_ATOMICMODULE_H_

#include <gef/core/binding/Common.h>
#include <gef/core/binding/Context.h>
#include <gef/core/system/Error.h>
#include <expected>
#include <filesystem>
#include <utility>

namespace gef {

using ModuleHandle = void*;
using gef_execute_fn_t = void (*)(gef::Context&);

struct AtomicModule {
    ModuleHandle          handle   = nullptr;
    const gef_metadata_t* metadata = nullptr;
    gef_execute_fn_t      execute  = nullptr;

    AtomicModule() noexcept = default;
    AtomicModule(ModuleHandle h, const gef_metadata_t* m, gef_execute_fn_t e) noexcept
        : handle(h), metadata(m), execute(e) {}

    ~AtomicModule() noexcept;

    AtomicModule(const AtomicModule&) = delete;
    auto operator=(const AtomicModule&) -> AtomicModule& = delete;

    AtomicModule(AtomicModule&& other) noexcept
        : handle(other.handle), metadata(other.metadata), execute(other.execute) {
        other.handle   = nullptr;
        other.metadata = nullptr;
        other.execute  = nullptr;
    }

    auto operator=(AtomicModule&& other) noexcept -> AtomicModule& {
        if (this != &other) {
            std::swap(handle, other.handle);
            std::swap(metadata, other.metadata);
            std::swap(execute, other.execute);
        }
        return *this;
    }
};

[[nodiscard]] auto loadAtomicModule(const std::filesystem::path& path)
    -> std::expected<AtomicModule, Error>;

}

#endif
