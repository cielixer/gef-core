#include "gef/core/module/ModuleRegistry.h"

#include "AtomicModule.hpp"

#include <boost/system/system_error.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace gef {

namespace {

auto normalizePath(const std::filesystem::path& path) -> std::string {
    std::error_code ec;
    auto            canonical_path = std::filesystem::canonical(path, ec);
    if (ec) {
        return path.string();
    }
    return canonical_path.string();
}

} // namespace

ModuleRegistry::~ModuleRegistry() = default;

auto registryAdd(ModuleRegistry& registry, ModuleDef def) -> ModuleId {
    if (def.name.empty()) [[unlikely]] {
        throw std::invalid_argument("Module name cannot be empty");
    }
    if (registry.name_to_id.contains(def.name)) [[unlikely]] {
        throw std::invalid_argument("Module already registered: " + def.name);
    }

    ModuleId id = static_cast<ModuleId>(registry.defs.size());
    registry.name_to_id[def.name] = id;
    registry.defs.push_back(std::move(def));
    return id;
}

auto registryGet(const ModuleRegistry& registry, ModuleId id)
    -> std::expected<const ModuleDef*, Error> {
    if (id >= registry.defs.size()) [[unlikely]] {
        return std::unexpected(Error{
            ErrorCode::ModuleNotFound,
            "Module ID out of range: " + std::to_string(id),
        });
    }
    return &registry.defs[id];
}

auto registryGet(ModuleRegistry& registry, ModuleId id)
    -> std::expected<ModuleDef*, Error> {
    if (id >= registry.defs.size()) [[unlikely]] {
        return std::unexpected(Error{
            ErrorCode::ModuleNotFound,
            "Module ID out of range: " + std::to_string(id),
        });
    }
    return &registry.defs[id];
}

auto registryFind(const ModuleRegistry& registry, std::string_view name)
    -> std::expected<ModuleId, Error> {
    auto it = registry.name_to_id.find(std::string(name));
    if (it == registry.name_to_id.end()) {
        return std::unexpected(Error{
            ErrorCode::ModuleNotFound,
            std::string("Module not found: ") + std::string(name),
        });
    }
    return it->second;
}

auto registrySize(const ModuleRegistry& registry) noexcept -> std::size_t {
    return registry.defs.size();
}

auto loadAtomicModule(ModuleRegistry& registry, const std::filesystem::path& path)
    -> std::expected<std::string, Error> {
    if (path.empty()) [[unlikely]] {
        throw std::invalid_argument("Module path cannot be empty");
    }

    if (!std::filesystem::exists(path)) [[unlikely]] {
        return std::unexpected(Error{
            ErrorCode::FileNotFound,
            "Module file does not exist: " + path.string(),
        });
    }

    if (!std::filesystem::is_regular_file(path)) [[unlikely]] {
        return std::unexpected(Error{
            ErrorCode::InvalidFileType,
            "Module path is not a regular file: " + path.string(),
        });
    }

    std::string norm_path = normalizePath(path);

    boost::dll::shared_library library;
    try {
        library.load(norm_path, boost::dll::load_mode::rtld_lazy | boost::dll::load_mode::rtld_local);
    } catch (const boost::system::system_error& e) {
        return std::unexpected(Error{
            ErrorCode::FileNotFound,
            "Failed to load library: " + std::string(e.what()),
        });
    }

    using GetMetadataFn = const gef_metadata_t* (*)();
    GetMetadataFn get_metadata = nullptr;
    try {
        get_metadata = library.get<const gef_metadata_t*()>("gef_get_metadata");
    } catch (const boost::system::system_error&) {
        return std::unexpected(Error{
            ErrorCode::SymbolNotFound,
            "Symbol 'gef_get_metadata' not found",
        });
    }

    const gef_metadata_t* metadata = get_metadata();
    if (!metadata || !metadata->module_name) [[unlikely]] {
        return std::unexpected(Error{
            ErrorCode::MetadataInvalid,
            "Module metadata or module_name is null",
        });
    }

    using ExecuteFn = void (*)(Context&);
    ExecuteFn execute = nullptr;
    try {
        execute = library.get<void(Context&)>("gef_execute");
    } catch (const boost::system::system_error&) {
        return std::unexpected(Error{
            ErrorCode::SymbolNotFound,
            "Symbol 'gef_execute' not found",
        });
    }

    std::string name(metadata->module_name);

    if (auto it = registry.atomic_name_to_path.find(name);
        it != registry.atomic_name_to_path.end()) {
        std::string old_path = it->second;
        if (auto old_it = registry.atomic_modules.find(name);
            old_it != registry.atomic_modules.end()) {
            registry.atomic_modules.erase(old_it);
        }
        registry.atomic_path_to_name.erase(old_path);
        registry.atomic_name_to_path.erase(it);
    }

    if (auto it = registry.atomic_path_to_name.find(norm_path);
        it != registry.atomic_path_to_name.end()) {
        std::string old_name = it->second;
        if (auto old_it = registry.atomic_modules.find(old_name);
            old_it != registry.atomic_modules.end()) {
            registry.atomic_modules.erase(old_it);
        }
        registry.atomic_name_to_path.erase(old_name);
        registry.atomic_path_to_name.erase(it);
    }

    registry.atomic_modules.insert_or_assign(
        name, createAtomicModuleWithLibrary(std::move(library), metadata, execute));
    registry.atomic_name_to_path[name]      = norm_path;
    registry.atomic_path_to_name[norm_path] = name;

    spdlog::info("Loaded or reloaded atomic module '{}' from {}", name, norm_path);
    return name;
}

auto getAtomicModule(const ModuleRegistry& registry, std::string_view name) noexcept
    -> std::expected<const AtomicModule*, Error> {
    auto it = registry.atomic_modules.find(std::string(name));
    if (it == registry.atomic_modules.end()) {
        return std::unexpected(Error{
            ErrorCode::ModuleNotFound,
            std::string("Atomic module not found: ") + std::string(name),
        });
    }
    return &it->second;
}

auto takeAtomicModule(ModuleRegistry& registry, std::string_view name)
    -> std::expected<AtomicModule, Error> {
    auto it = registry.atomic_modules.find(std::string(name));
    if (it == registry.atomic_modules.end()) {
        return std::unexpected(Error{
            ErrorCode::ModuleNotFound,
            std::string("Atomic module not found: ") + std::string(name),
        });
    }
    AtomicModule module = std::move(it->second);
    registry.atomic_modules.erase(it);
    return module;
}

auto atomicModuleNames(const ModuleRegistry& registry) -> std::vector<std::string> {
    std::vector<std::string> names;
    names.reserve(registry.atomic_modules.size());
    for (const auto& [name, _] : registry.atomic_modules) {
        names.push_back(name);
    }
    return names;
}

} // namespace gef
