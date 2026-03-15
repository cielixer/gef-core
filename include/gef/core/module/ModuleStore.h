#ifndef GEF_MODULESTORE_H_
#define GEF_MODULESTORE_H_

#include <gef/core/system/Error.h>
#include <gef/core/module/Module.h>
#include <cstddef>
#include <deque>
#include <expected>
#include <functional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace gef {

struct TransparentStringHash {
    using is_transparent = void;

    [[nodiscard]] auto operator()(std::string_view value) const noexcept -> std::size_t {
        return std::hash<std::string_view>{}(value);
    }
};

struct ModuleStore {
    mutable std::shared_mutex mutex;
    std::deque<ModuleDef> defs;
    std::unordered_map<std::string, ModuleId,
                       TransparentStringHash, std::equal_to<>> name_to_id;
};

[[nodiscard]] auto registerModule(ModuleStore& store, ModuleDef def) -> ModuleId;

[[nodiscard]] auto getModule(const ModuleStore& store, ModuleId id) noexcept
    -> std::expected<const ModuleDef*, Error>;

[[nodiscard]] auto findModule(const ModuleStore& store,
                              std::string_view name) noexcept
    -> std::expected<ModuleId, Error>;

[[nodiscard]] auto registerCompositeModule(
    ModuleStore& store,
    std::string name,
    ModuleSignature signature,
    CompositeModule composite
) -> std::expected<ModuleId, Error>;

} // namespace gef

#endif
