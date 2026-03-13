#include <gef/core/module/ModuleRegistry.h>
#include <spdlog/spdlog.h>
#include <dlfcn.h>
#include <stdexcept>
#include <system_error>

namespace fs = std::filesystem;

namespace gef {

ModuleRegistry::~ModuleRegistry() noexcept {
    std::vector<std::string> names_to_unload;
    names_to_unload.reserve(atomic_modules_.size());
    for (const auto& [name, _] : atomic_modules_) {
        names_to_unload.push_back(name);
    }
    for (const auto& name : names_to_unload) {
        auto it = atomic_modules_.find(name);
        if (it == atomic_modules_.end()) {
            continue;
        }

        auto handle = it->second.handle;
        atomic_modules_.erase(it);

        auto path_it = atomic_name_to_path_.find(name);
        if (path_it != atomic_name_to_path_.end()) {
            atomic_path_to_name_.erase(path_it->second);
            atomic_name_to_path_.erase(path_it);
        }

        if (handle && dlclose(handle) != 0) {
            spdlog::warn("Failed to unload module '{}': {}", name, dlerror());
        }
    }
}

auto ModuleRegistry::add(ModuleDef def) -> ModuleId {
    if (def.name.empty()) [[unlikely]] {
        throw std::invalid_argument("Module name cannot be empty");
    }

    if (name_to_id_.contains(def.name)) [[unlikely]] {
        throw std::invalid_argument("Duplicate module name: " + def.name);
    }

    auto id = static_cast<ModuleId>(defs_.size());
    name_to_id_[def.name] = id;
    defs_.push_back(std::move(def));
    return id;
}

auto ModuleRegistry::get(ModuleId id) const noexcept
    -> std::expected<const ModuleDef*, Error> {
    if (id >= defs_.size()) {
        return std::unexpected(Error{ErrorCode::ModuleNotFound,
                                     "Module ID out of range"});
    }
    return &defs_[id];
}

auto ModuleRegistry::get(ModuleId id) noexcept
    -> std::expected<ModuleDef*, Error> {
    if (id >= defs_.size()) {
        return std::unexpected(Error{ErrorCode::ModuleNotFound,
                                     "Module ID out of range"});
    }
    return &defs_[id];
}

auto ModuleRegistry::find(std::string_view qualified_name) const noexcept
    -> std::expected<ModuleId, Error> {
    auto it = name_to_id_.find(std::string(qualified_name));
    if (it == name_to_id_.end()) {
        return std::unexpected(Error{ErrorCode::ModuleNotFound,
                                     std::string("Module not found: ").append(qualified_name)});
    }
    return it->second;
}

auto ModuleRegistry::size() const noexcept -> std::size_t {
    return defs_.size();
}

auto loadAtomicModule(ModuleRegistry& registry, const fs::path& path)
    -> std::expected<std::string, Error> {
    const auto unload_atomic_by_name = [&registry](std::string_view name) {
        auto it = registry.atomic_modules_.find(std::string(name));
        if (it == registry.atomic_modules_.end()) {
            return;
        }

        auto handle = it->second.handle;
        auto module_name = it->first;
        registry.atomic_modules_.erase(it);

        auto path_it = registry.atomic_name_to_path_.find(module_name);
        if (path_it != registry.atomic_name_to_path_.end()) {
            registry.atomic_path_to_name_.erase(path_it->second);
            registry.atomic_name_to_path_.erase(path_it);
        }

        if (handle && dlclose(handle) != 0) {
#ifndef _WIN32
            spdlog::warn("Failed to unload module '{}': {}", module_name, dlerror());
#endif
        }
    };

    if (path.empty()) [[unlikely]] {
        throw std::invalid_argument("Module path cannot be empty");
    }

    if (!fs::exists(path)) [[unlikely]] {
        return std::unexpected(Error{ErrorCode::FileNotFound,
                                     "Module file does not exist: " + path.string()});
    }

    if (!fs::is_regular_file(path)) [[unlikely]] {
        return std::unexpected(Error{ErrorCode::InvalidFileType,
                                     "Module path is not a regular file: " + path.string()});
    }

    std::error_code ec;
    auto normalized_path = fs::absolute(path, ec).lexically_normal().string();
    if (ec) {
        normalized_path = path.lexically_normal().string();
    }

    auto hot_reload_it = registry.atomic_path_to_name_.find(normalized_path);
    if (hot_reload_it != registry.atomic_path_to_name_.end()) {
        unload_atomic_by_name(hot_reload_it->second);
    }

    auto handle = dlopen(path.c_str(), RTLD_NOW);
    if (!handle) [[unlikely]] {
        return std::unexpected(Error{ErrorCode::LoadFailed,
                                     std::string("Failed to load module: ") + dlerror()});
    }

    using GetMetadataFunc = const gef_metadata_t* (*)();

    auto get_metadata_sym = dlsym(handle, "gef_get_metadata");
    if (!get_metadata_sym) [[unlikely]] {
        dlclose(handle);
        return std::unexpected(Error{ErrorCode::SymbolNotFound,
                                     std::string("Failed to find symbol: ") + dlerror()});
    }

    auto get_metadata = reinterpret_cast<GetMetadataFunc>(get_metadata_sym);
    auto metadata = get_metadata();
    if (!metadata) [[unlikely]] {
        dlclose(handle);
        return std::unexpected(Error{ErrorCode::MetadataInvalid,
                                     "Failed to retrieve module metadata"});
    }

    auto execute_sym = dlsym(handle, "gef_execute");
    if (!execute_sym) [[unlikely]] {
        dlclose(handle);
        return std::unexpected(Error{ErrorCode::SymbolNotFound,
                                     std::string("Failed to find symbol: ") + dlerror()});
    }

    auto execute = reinterpret_cast<gef_execute_fn_t>(execute_sym);
    std::string name{metadata->module_name};

    if (registry.atomic_modules_.contains(name)) {
        unload_atomic_by_name(name);
    }

    registry.atomic_modules_.insert_or_assign(name, AtomicModule{handle, metadata, execute});
    registry.atomic_name_to_path_.insert_or_assign(name, normalized_path);
    registry.atomic_path_to_name_.insert_or_assign(normalized_path, name);

    spdlog::info("Loaded or reloaded atomic module '{}' from {}", name, path.string());
    return name;
}

auto getAtomicModule(const ModuleRegistry& registry,
                     std::string_view name) noexcept
    -> std::expected<const AtomicModule*, Error> {
    auto it = registry.atomic_modules_.find(std::string(name));
    if (it == registry.atomic_modules_.end()) {
        return std::unexpected(Error{ErrorCode::ModuleNotFound,
                                     std::string("Atomic module not found: ").append(name)});
    }
    return &it->second;
}

auto atomicModuleNames(const ModuleRegistry& registry) -> std::vector<std::string> {
    std::vector<std::string> result;
    result.reserve(registry.atomic_modules_.size());
    for (const auto& [name, _] : registry.atomic_modules_) {
        result.push_back(name);
    }
    return result;
}

} // namespace gef
