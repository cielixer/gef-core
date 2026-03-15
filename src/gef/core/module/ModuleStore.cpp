#include <gef/core/module/ModuleStore.h>
#include <format>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>

namespace gef {

auto registerModule(ModuleStore& store, ModuleDef def) -> ModuleId {
    if (def.name.empty()) [[unlikely]] {
        throw std::invalid_argument("Module name cannot be empty");
    }

    std::unique_lock lock(store.mutex);

    if (store.name_to_id.contains(def.name)) [[unlikely]] {
        throw std::invalid_argument("Duplicate module name: " + def.name);
    }

    auto id = static_cast<ModuleId>(store.defs.size());
    store.defs.push_back(std::move(def));

    try {
        store.name_to_id.emplace(store.defs.back().name, id);
    } catch (...) {
        store.defs.pop_back();
        throw;
    }

    return id;
}

auto getModule(const ModuleStore& store, ModuleId id) noexcept
    -> std::expected<const ModuleDef*, Error> {
    std::shared_lock lock(store.mutex);

    if (id >= store.defs.size()) {
        return std::unexpected(Error{ErrorCode::ModuleNotFound,
                                     "Module ID out of range"});
    }
    return &store.defs[id];
}

auto findModule(const ModuleStore& store, std::string_view name) noexcept
    -> std::expected<ModuleId, Error> {
    std::shared_lock lock(store.mutex);

    auto it = store.name_to_id.find(name);
    if (it == store.name_to_id.end()) {
        return std::unexpected(Error{ErrorCode::ModuleNotFound,
                                     std::format("Module not found: {}", name)});
    }
    return it->second;
}

} // namespace gef
