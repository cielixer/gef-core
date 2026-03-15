#ifndef GEF_COMPOSITEMODULE_H_
#define GEF_COMPOSITEMODULE_H_

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace gef {

// Forward declaration
struct Error;

using ModuleId = std::uint32_t;

inline constexpr ModuleId kInvalidModuleId = static_cast<ModuleId>(-1);

using CompositeChildId = std::uint32_t;

inline constexpr CompositeChildId kInvalidCompositeChildId =
    static_cast<CompositeChildId>(-1);

/// A child instance within a composite module.
/// Stores the instance name for lookup and the module_id for definition lookup.
struct CompositeChild {
    std::string instance_name;
    ModuleId    module_id;
};

/// An edge connecting two child instances (child-local ids, not module-global).
struct CompositeEdge {
    CompositeChildId from;
    CompositeChildId to;
};

/// Canonical representation of a composite module topology.
/// Stores only children + edges; no lookup maps, no caches, no endpoint strings.
struct CompositeModule {
    std::vector<CompositeChild> children;
    std::vector<CompositeEdge>  edges;
};

[[nodiscard]] auto addChild(CompositeModule& cm, std::string instance_name,
                             ModuleId module_id) noexcept
    -> std::expected<CompositeChildId, Error>;

[[nodiscard]] auto findChild(const CompositeModule& cm,
                              std::string_view instance_name) noexcept
    -> std::expected<CompositeChildId, Error>;

[[nodiscard]] auto addEdge(CompositeModule& cm, CompositeChildId from,
                            CompositeChildId to) noexcept
    -> std::expected<void, Error>;

[[nodiscard]] auto validateCompositeTopology(const CompositeModule& cm) noexcept
    -> std::expected<void, Error>;

[[nodiscard]] auto topologicalOrder(const CompositeModule& cm) noexcept
    -> std::expected<std::vector<CompositeChildId>, Error>;

}

#endif
