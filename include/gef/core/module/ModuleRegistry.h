#ifndef GEF_MODULEREGISTRY_H_
#define GEF_MODULEREGISTRY_H_

#include "gef/core/module/ModuleVariant.h"
#include "gef/core/system/Error.h"

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace gef {

class ModuleRegistry {
  public:
    ModuleRegistry() noexcept = default;
    ~ModuleRegistry();

    ModuleRegistry(const ModuleRegistry&) = delete;
    auto operator=(const ModuleRegistry&) -> ModuleRegistry& = delete;
    ModuleRegistry(ModuleRegistry&&) noexcept = default;
    auto operator=(ModuleRegistry&&) noexcept -> ModuleRegistry& = default;

    [[nodiscard]] auto add(ModuleDef def) -> ModuleId;

    [[nodiscard]] auto get(ModuleId id) const
        -> std::expected<const ModuleDef*, Error>;

    [[nodiscard]] auto get(ModuleId id)
        -> std::expected<ModuleDef*, Error>;

    [[nodiscard]] auto find(std::string_view qualified_name) const
        -> std::expected<ModuleId, Error>;

    [[nodiscard]] auto size() const noexcept -> std::size_t;

  private:
    friend auto loadAtomicModule(ModuleRegistry& registry,
                                 const std::filesystem::path& path)
        -> std::expected<std::string, Error>;

    friend auto getAtomicModule(const ModuleRegistry& registry,
                                std::string_view name) noexcept
        -> std::expected<const AtomicModule*, Error>;

    friend auto takeAtomicModule(ModuleRegistry& registry,
                                 std::string_view name)
        -> std::expected<AtomicModule, Error>;

    friend auto atomicModuleNames(const ModuleRegistry& registry)
        -> std::vector<std::string>;

    std::vector<ModuleDef> defs_;
    std::unordered_map<std::string, ModuleId> name_to_id_;

    std::unordered_map<std::string, AtomicModule> atomic_modules_;
    std::unordered_map<std::string, std::string> atomic_name_to_path_;
    std::unordered_map<std::string, std::string> atomic_path_to_name_;
};

[[nodiscard]] auto loadAtomicModule(ModuleRegistry& registry,
                                    const std::filesystem::path& path)
    -> std::expected<std::string, Error>;

[[nodiscard]] auto getAtomicModule(const ModuleRegistry& registry,
                                   std::string_view name) noexcept
    -> std::expected<const AtomicModule*, Error>;

[[nodiscard]] auto takeAtomicModule(ModuleRegistry& registry,
                                    std::string_view name)
    -> std::expected<AtomicModule, Error>;

[[nodiscard]] auto atomicModuleNames(const ModuleRegistry& registry)
    -> std::vector<std::string>;

} // namespace gef

#endif
