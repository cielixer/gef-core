#ifndef GEF_SYSTEM_H_
#define GEF_SYSTEM_H_

#include <gef/core/binding/Context.h>
#include <gef/core/module/ModuleRegistry.h>
#include <gef/core/module/ModuleVariant.h>
#include <expected>
#include <filesystem>
#include <string_view>

namespace gef {

struct System {
    System() noexcept = default;
    ~System() noexcept = default;

    System(const System&) = delete;
    auto operator=(const System&) -> System& = delete;
    System(System&&) noexcept = default;
    auto operator=(System&&) noexcept -> System& = default;

    ModuleRegistry module_registry;
};

[[nodiscard]] auto loadModule(System& system, const std::filesystem::path& path)
    -> std::expected<ModuleId, Error>;

[[nodiscard]] auto executeModule(System& system, std::string_view name, Context& ctx)
    -> std::expected<void, Error>;

[[nodiscard]] auto executeModule(System& system, ModuleId id, Context& ctx)
    -> std::expected<void, Error>;

} // namespace gef

#endif
