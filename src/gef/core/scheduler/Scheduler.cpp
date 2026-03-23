#include <gef/core/scheduler/Scheduler.h>
#include <algorithm>
#include <unordered_map>
#include <vector>

namespace gef {

auto topologicalSort(
    const std::vector<std::string>& nodes,
    const std::vector<std::pair<std::string, std::string>>& edges)
    -> std::expected<std::vector<std::string>, Error> {

    if (nodes.empty()) {
        return std::vector<std::string>{};
    }

    std::unordered_map<std::string, std::vector<std::string>> adjacency;
    std::unordered_map<std::string, int> in_degree;

    for (const auto& node : nodes) {
        in_degree[node] = 0;
        adjacency[node];
    }

    for (const auto& [source, destination] : edges) {
        adjacency[source].push_back(destination);
        in_degree[destination]++;
    }

    // Collect zero in-degree nodes and sort for deterministic (lexicographic) tie-breaking
    std::vector<std::string> queue;
    for (const auto& [node, degree] : in_degree) {
        if (degree == 0) {
            queue.push_back(node);
        }
    }
    std::sort(queue.begin(), queue.end());

    std::vector<std::string> result;
    result.reserve(nodes.size());

    // Kahn's algorithm: always pick lexicographically smallest ready node
    while (!queue.empty()) {
        auto current = std::move(queue.front());
        queue.erase(queue.begin());
        result.push_back(current);

        std::vector<std::string> newly_ready;
        for (const auto& neighbor : adjacency[current]) {
            in_degree[neighbor]--;
            if (in_degree[neighbor] == 0) {
                newly_ready.push_back(neighbor);
            }
        }

        if (!newly_ready.empty()) {
            std::sort(newly_ready.begin(), newly_ready.end());
            for (auto& n : newly_ready) {
                auto pos = std::lower_bound(queue.begin(), queue.end(), n);
                queue.insert(pos, std::move(n));
            }
        }
    }

    if (result.size() != nodes.size()) [[unlikely]] {
        return std::unexpected(Error{
            ErrorCode::CycleDetected,
            "Cycle detected in module graph",
        });
    }

    return result;
}

} // namespace gef
