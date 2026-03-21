#include "gef/core/module/ModuleRegistry.h"

#include "gef/core/module/AtomicModule.h"

#include <dlfcn.h>
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

auto ModuleRegistry::add(ModuleDef def) -> ModuleId {
    if (def.name.empty()) [[unlikely]] {
        throw std::invalid_argument("Module name cannot be empty");
    }
    if (name_to_id_.contains(def.name)) [[unlikely]] {
        throw std::invalid_argument("Module already registered: " + def.name);
    }

    ModuleId id      = static_cast<ModuleId>(defs_.size());
    name_to_id_[def.name] = id;
    defs_.push_back(std::move(def));
    return id;
}

auto ModuleRegistry::get(ModuleId id) const
    -> std::expected<const ModuleDef*, Error> {
    if (id >= defs_.size()) [[unlikely]] {
        return std::unexpected(Error{
            ErrorCode::ModuleNotFound,
            "Module ID out of range: " + std::to_string(id),
        });
    }
    return &defs_[id];
}

auto ModuleRegistry::get(ModuleId id) -> std::expected<ModuleDef*, Error> {
    if (id >= defs_.size()) [[unlikely]] {
        return std::unexpected(Error{
            ErrorCode::ModuleNotFound,
            "Module ID out of range: " + std::to_string(id),
        });
    }
    return &defs_[id];
}

auto ModuleRegistry::find(std::string_view name) const
    -> std::expected<ModuleId, Error> {
    auto it = name_to_id_.find(std::string(name));
    if (it == name_to_id_.end()) {
        return std::unexpected(Error{
            ErrorCode::ModuleNotFound,
            std::string("Module not found: ") + std::string(name),
        });
    }
    return it->second;
}

auto ModuleRegistry::size() const noexcept -> std::size_t {
    return defs_.size();
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

    void* handle = dlopen(norm_path.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (!handle) [[unlikely]] {
        return std::unexpected(Error{
            ErrorCode::FileNotFound,
            std::string("Failed to dlopen: ") + dlerror(),
        });
    }

    using GetMetadataFn = const gef_metadata_t* (*)();
    auto get_metadata   = reinterpret_cast<GetMetadataFn>(dlsym(handle, "gef_get_metadata"));
    if (!get_metadata) [[unlikely]] {
        dlclose(handle);
        return std::unexpected(Error{
            ErrorCode::SymbolNotFound,
            "Symbol 'gef_get_metadata' not found",
        });
    }

    const gef_metadata_t* metadata = get_metadata();
    if (!metadata || !metadata->module_name) [[unlikely]] {
        dlclose(handle);
        return std::unexpected(Error{
            ErrorCode::MetadataInvalid,
            "Module metadata or module_name is null",
        });
    }

    using ExecuteFn = void (*)(Context&);
    auto execute    = reinterpret_cast<ExecuteFn>(dlsym(handle, "gef_execute"));
    if (!execute) [[unlikely]] {
        dlclose(handle);
        return std::unexpected(Error{
            ErrorCode::SymbolNotFound,
            "Symbol 'gef_execute' not found",
        });
    }

    std::string name(metadata->module_name);

    if (auto it = registry.atomic_name_to_path_.find(name);
        it != registry.atomic_name_to_path_.end()) {
        std::string old_path = it->second;
        if (auto old_it = registry.atomic_modules_.find(name);
            old_it != registry.atomic_modules_.end()) {
            registry.atomic_modules_.erase(old_it);
        }
        registry.atomic_path_to_name_.erase(old_path);
        registry.atomic_name_to_path_.erase(it);
    }

    if (auto it = registry.atomic_path_to_name_.find(norm_path);
        it != registry.atomic_path_to_name_.end()) {
        std::string old_name = it->second;
        if (auto old_it = registry.atomic_modules_.find(old_name);
            old_it != registry.atomic_modules_.end()) {
            registry.atomic_modules_.erase(old_it);
        }
        registry.atomic_name_to_path_.erase(old_name);
        registry.atomic_path_to_name_.erase(it);
    }

    registry.atomic_modules_.insert_or_assign(
        name, createAtomicModule(handle, metadata, execute));
    registry.atomic_name_to_path_[name]      = norm_path;
    registry.atomic_path_to_name_[norm_path] = name;

    spdlog::info("Loaded or reloaded atomic module '{}' from {}", name, norm_path);
    return name;
}

auto getAtomicModule(const ModuleRegistry& registry, std::string_view name) noexcept
    -> std::expected<const AtomicModule*, Error> {
    auto it = registry.atomic_modules_.find(std::string(name));
    if (it == registry.atomic_modules_.end()) {
        return std::unexpected(Error{
            ErrorCode::ModuleNotFound,
            std::string("Atomic module not found: ") + std::string(name),
        });
    }
    return &it->second;
}

auto takeAtomicModule(ModuleRegistry& registry, std::string_view name)
    -> std::expected<AtomicModule, Error> {
    auto it = registry.atomic_modules_.find(std::string(name));
    if (it == registry.atomic_modules_.end()) {
        return std::unexpected(Error{
            ErrorCode::ModuleNotFound,
            std::string("Atomic module not found: ") + std::string(name),
        });
    }
    AtomicModule module = std::move(it->second);
    registry.atomic_modules_.erase(it);
    return module;
}

auto atomicModuleNames(const ModuleRegistry& registry) -> std::vector<std::string> {
    std::vector<std::string> names;
    names.reserve(registry.atomic_modules_.size());
    for (const auto& [name, _] : registry.atomic_modules_) {
        names.push_back(name);
    }
    return names;
}

} // namespace gef
