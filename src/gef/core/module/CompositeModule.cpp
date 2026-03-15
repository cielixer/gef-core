#include <gef/core/module/CompositeModule.h>
#include <gef/core/module/ModuleStore.h>
#include <gef/core/system/Error.h>
#include <format>
#include <map>
#include <set>
#include <unordered_set>

namespace {

struct CompositeDfsFrame {
    gef::ModuleId             module_id;
    const gef::CompositeModule* composite;
    std::size_t               next_child_index;
    std::string_view          module_name;
};

auto validateReferencedCompositeDfs(
    const gef::ModuleStore& store,
    gef::ModuleId root_module_id,
    std::unordered_set<gef::ModuleId>& validated_composites
) -> std::expected<void, gef::Error> {
    if (validated_composites.contains(root_module_id)) {
        return {};
    }

    std::unordered_set<gef::ModuleId> recursion_stack;
    std::vector<CompositeDfsFrame> dfs_stack;

    auto pushComposite = [&](gef::ModuleId module_id) -> std::expected<void, gef::Error> {
        if (validated_composites.contains(module_id)) {
            return {};
        }

        auto def_result = gef::getModule(store, module_id);
        if (!def_result) {
            return std::unexpected(gef::Error{
                gef::ErrorCode::ModuleNotFound,
                std::format(
                    "Referenced module id {} not found while validating nested composites",
                    module_id)});
        }

        const auto* def = *def_result;
        if (!std::holds_alternative<gef::CompositeModule>(def->variant)) {
            validated_composites.insert(module_id);
            return {};
        }

        const auto& composite = std::get<gef::CompositeModule>(def->variant);
        if (auto topology_result = gef::validateCompositeTopology(composite); !topology_result) {
            return std::unexpected(gef::Error{
                gef::ErrorCode::InvalidTopology,
                std::format(
                    "Referenced composite '{}' (id {}) has invalid topology: {}",
                    def->name,
                    module_id,
                    topology_result.error().message)});
        }

        recursion_stack.insert(module_id);
        dfs_stack.push_back(CompositeDfsFrame{
            .module_id = module_id,
            .composite = &composite,
            .next_child_index = 0,
            .module_name = def->name,
        });
        return {};
    };

    if (auto push_result = pushComposite(root_module_id); !push_result) {
        return std::unexpected(push_result.error());
    }

    while (!dfs_stack.empty()) {
        auto& frame = dfs_stack.back();

        if (frame.next_child_index >= frame.composite->children.size()) {
            recursion_stack.erase(frame.module_id);
            validated_composites.insert(frame.module_id);
            dfs_stack.pop_back();
            continue;
        }

        const auto& child = frame.composite->children[frame.next_child_index++];

        auto child_def_result = gef::getModule(store, child.module_id);
        if (!child_def_result) {
            return std::unexpected(gef::Error{
                gef::ErrorCode::ModuleNotFound,
                std::format(
                    "Referenced module id {} (instance '{}') not found while validating composite '{}'",
                    child.module_id,
                    child.instance_name,
                    frame.module_name)});
        }

        const auto* child_def = *child_def_result;
        if (!std::holds_alternative<gef::CompositeModule>(child_def->variant)) {
            continue;
        }

        if (validated_composites.contains(child.module_id)) {
            continue;
        }

        if (recursion_stack.contains(child.module_id)) {
            return std::unexpected(gef::Error{
                gef::ErrorCode::InvalidTopology,
                std::format(
                    "Cycle detected in registered composite references (module '{}' -> '{}')",
                    frame.module_name,
                    child_def->name)});
        }

        if (auto push_result = pushComposite(child.module_id); !push_result) {
            return std::unexpected(push_result.error());
        }
    }

    return {};
}

}

namespace gef {

auto addChild(CompositeModule& cm, std::string instance_name,
               ModuleId module_id) noexcept
    -> std::expected<CompositeChildId, Error> {
    if (instance_name.empty()) {
        return std::unexpected(
            Error{ErrorCode::InvalidTopology,
                  "Instance name cannot be empty"});
    }

    if (module_id == kInvalidModuleId) {
        return std::unexpected(
            Error{ErrorCode::InvalidTopology,
                  "Module ID cannot be kInvalidModuleId"});
    }

    auto local_id = static_cast<CompositeChildId>(cm.children.size());
    cm.children.push_back({std::move(instance_name), module_id});
    return local_id;
}

auto findChild(const CompositeModule& cm,
                std::string_view instance_name) noexcept
    -> std::expected<CompositeChildId, Error> {
    for (size_t i = 0; i < cm.children.size(); ++i) {
        if (cm.children[i].instance_name == instance_name) {
            return static_cast<CompositeChildId>(i);
        }
    }

    return std::unexpected(
        Error{ErrorCode::ModuleNotFound,
              std::format("Child instance not found: {}", instance_name)});
}

auto addEdge(CompositeModule& cm, CompositeChildId from,
              CompositeChildId to) noexcept
    -> std::expected<void, Error> {
    auto num_children = static_cast<CompositeChildId>(cm.children.size());

    // Reject out-of-range ids
    if (from >= num_children) {
        return std::unexpected(
            Error{ErrorCode::InvalidTopology,
                  std::format("Source child id {} is out of range (total children: {})",
                              from, num_children)});
    }

    if (to >= num_children) {
        return std::unexpected(
            Error{ErrorCode::InvalidTopology,
                  std::format("Target child id {} is out of range (total children: {})",
                              to, num_children)});
    }

    // Reject self-edges
    if (from == to) {
        return std::unexpected(
            Error{ErrorCode::InvalidTopology,
                  std::format("Self-edge not allowed (child {})", from)});
    }

    // Reject duplicate edges
    for (const auto& edge : cm.edges) {
        if (edge.from == from && edge.to == to) {
            return std::unexpected(
                Error{ErrorCode::InvalidTopology,
                      std::format("Duplicate edge ({} -> {})", from, to)});
        }
    }

    // Append valid edge
    cm.edges.push_back({from, to});
    return {};
}

auto validateCompositeTopology(const CompositeModule& cm) noexcept
    -> std::expected<void, Error> {
    if (cm.children.empty()) {
        return {};
    }

    auto num_children = static_cast<CompositeChildId>(cm.children.size());

    std::set<std::string_view> seen_names;
    for (const auto& child : cm.children) {
        if (!seen_names.insert(child.instance_name).second) {
            return std::unexpected(
                Error{ErrorCode::InvalidTopology,
                      std::format("Duplicate instance name: {}", child.instance_name)});
        }
    }

    std::set<std::pair<CompositeChildId, CompositeChildId>> seen_edges;
    for (const auto& edge : cm.edges) {
        if (edge.from >= num_children) {
            return std::unexpected(
                Error{ErrorCode::InvalidTopology,
                      std::format("Source child id {} is out of range (total children: {})",
                                  edge.from, num_children)});
        }

        if (edge.to >= num_children) {
            return std::unexpected(
                Error{ErrorCode::InvalidTopology,
                      std::format("Target child id {} is out of range (total children: {})",
                                  edge.to, num_children)});
        }

        if (edge.from == edge.to) {
            return std::unexpected(
                Error{ErrorCode::InvalidTopology,
                      std::format("Self-edge not allowed (child {})", edge.from)});
        }

        if (!seen_edges.insert({edge.from, edge.to}).second) {
            return std::unexpected(
                Error{ErrorCode::InvalidTopology,
                      std::format("Duplicate edge ({} -> {})", edge.from, edge.to)});
        }
    }

    std::map<CompositeChildId, std::vector<CompositeChildId>> adjacency;
    std::map<CompositeChildId, int> in_degree;

    for (CompositeChildId i = 0; i < num_children; ++i) {
        in_degree[i] = 0;
        adjacency[i];
    }

    for (const auto& edge : cm.edges) {
        adjacency[edge.from].push_back(edge.to);
        in_degree[edge.to]++;
    }

    auto cmp = [&cm](CompositeChildId a, CompositeChildId b) {
        return cm.children[a].instance_name < cm.children[b].instance_name;
    };

    std::set<CompositeChildId, decltype(cmp)> queue(cmp);
    for (const auto& [child_id, degree] : in_degree) {
        if (degree == 0) {
            queue.insert(child_id);
        }
    }

    size_t processed = 0;
    while (!queue.empty()) {
        auto current = *queue.begin();
        queue.erase(queue.begin());
        processed++;

        for (const auto& neighbor : adjacency[current]) {
            in_degree[neighbor]--;
            if (in_degree[neighbor] == 0) {
                queue.insert(neighbor);
            }
        }
    }

    if (processed != num_children) {
        return std::unexpected(
            Error{ErrorCode::InvalidTopology,
                  std::format("Cycle detected in composite module DAG (processed {}/{} children)",
                              processed, num_children)});
    }

    return {};
}

auto topologicalOrder(const CompositeModule& cm) noexcept
    -> std::expected<std::vector<CompositeChildId>, Error> {
    if (auto result = validateCompositeTopology(cm); !result) {
        return std::unexpected(result.error());
    }

    std::vector<CompositeChildId> result;

    if (cm.children.empty()) {
        return result;
    }

    auto num_children = static_cast<CompositeChildId>(cm.children.size());

    std::map<CompositeChildId, std::vector<CompositeChildId>> adjacency;
    std::map<CompositeChildId, int> in_degree;

    for (CompositeChildId i = 0; i < num_children; ++i) {
        in_degree[i] = 0;
        adjacency[i];
    }

    for (const auto& edge : cm.edges) {
        adjacency[edge.from].push_back(edge.to);
        in_degree[edge.to]++;
    }

    auto cmp = [&cm](CompositeChildId a, CompositeChildId b) {
        return cm.children[a].instance_name < cm.children[b].instance_name;
    };

    std::set<CompositeChildId, decltype(cmp)> queue(cmp);
    for (const auto& [child_id, degree] : in_degree) {
        if (degree == 0) {
            queue.insert(child_id);
        }
    }

    while (!queue.empty()) {
        auto current = *queue.begin();
        queue.erase(queue.begin());
        result.push_back(current);

        for (const auto& neighbor : adjacency[current]) {
            in_degree[neighbor]--;
            if (in_degree[neighbor] == 0) {
                queue.insert(neighbor);
            }
        }
    }

    return result;
}

auto registerCompositeModule(
    ModuleStore& store,
    std::string name,
    ModuleSignature signature,
    CompositeModule composite
) -> std::expected<ModuleId, Error> {
    if (auto result = validateCompositeTopology(composite); !result) {
        return std::unexpected(result.error());
    }

    std::unordered_set<ModuleId> validated_composites;

    for (const auto& child : composite.children) {
        auto module_result = getModule(store, child.module_id);
        if (!module_result) {
            return std::unexpected(
                Error{ErrorCode::ModuleNotFound,
                      std::format("Child module id {} (instance '{}') not found in store",
                                  child.module_id, child.instance_name)});
        }

        if (std::holds_alternative<CompositeModule>((*module_result)->variant)) {
            auto nested_result =
                validateReferencedCompositeDfs(store, child.module_id, validated_composites);
            if (!nested_result) {
                return std::unexpected(std::move(nested_result.error()));
            }
        }
    }

    ModuleDef def{
        .name      = std::move(name),
        .signature = std::move(signature),
        .variant   = std::move(composite),
    };

    return registerModule(store, std::move(def));
}

} // namespace gef
