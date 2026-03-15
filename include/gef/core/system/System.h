#ifndef GEF_SYSTEM_H_
#define GEF_SYSTEM_H_

#include <gef/core/binding/Context.h>
#include <gef/core/module/ModuleStore.h>
#include <gef/core/module/Module.h>
#include <expected>
#include <filesystem>
#include <string_view>

namespace gef {

class System {
  public:
    System() noexcept = default;
    ~System() noexcept = default;

    System(const System&) = delete;
    auto operator=(const System&) -> System& = delete;
    System(System&&) noexcept = delete;
    auto operator=(System&&) noexcept -> System& = delete;

    [[nodiscard]] auto loadModule(const std::filesystem::path& path)
        -> std::expected<ModuleId, Error>;

    [[nodiscard]] auto executeModule(std::string_view name, Context& ctx)
        -> std::expected<void, Error>;

    [[nodiscard]] auto executeModule(ModuleId id, Context& ctx)
        -> std::expected<void, Error>;

    [[nodiscard]] auto moduleStore() const noexcept -> const ModuleStore&;
    [[nodiscard]] auto moduleStore() noexcept -> ModuleStore&;

  private:
    ModuleStore module_store_;
};

} // namespace gef

#endif
