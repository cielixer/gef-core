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

struct ModuleRegistry {
    ModuleRegistry() noexcept = default;
    ~ModuleRegistry();

    ModuleRegistry(const ModuleRegistry&) = delete;
    auto operator=(const ModuleRegistry&) -> ModuleRegistry& = delete;
    ModuleRegistry(ModuleRegistry&&) noexcept = default;
    auto operator=(ModuleRegistry&&) noexcept -> ModuleRegistry& = default;

    std::vector<ModuleDef> defs;
    std::unordered_map<std::string, ModuleId> name_to_id;

    std::unordered_map<std::string, AtomicModule> atomic_modules;
    std::unordered_map<std::string, std::string> atomic_name_to_path;
    std::unordered_map<std::string, std::string> atomic_path_to_name;
};

[[nodiscard]] auto registryAdd(ModuleRegistry& registry, ModuleDef def) -> ModuleId;

[[nodiscard]] auto registryGet(const ModuleRegistry& registry, ModuleId id)
    -> std::expected<const ModuleDef*, Error>;

[[nodiscard]] auto registryGet(ModuleRegistry& registry, ModuleId id)
    -> std::expected<ModuleDef*, Error>;

[[nodiscard]] auto registryFind(const ModuleRegistry& registry,
                                std::string_view qualified_name)
    -> std::expected<ModuleId, Error>;

[[nodiscard]] auto registrySize(const ModuleRegistry& registry) noexcept -> std::size_t;

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
