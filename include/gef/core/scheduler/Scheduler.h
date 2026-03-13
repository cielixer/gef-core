#ifndef GEF_SCHEDULER_H_
#define GEF_SCHEDULER_H_

#include <string>
#include <vector>
#include <utility>

namespace gef {

class Scheduler {
  public:
    /// Performs topological sort using Kahn's algorithm with deterministic (lexicographic) tie-breaking.
    /// @param nodes List of node identifiers
    /// @param edges List of (source, destination) pairs representing directed edges
    /// @return Vector of nodes in topological order
    /// @throws std::runtime_error if a cycle is detected
    [[nodiscard]] static std::vector<std::string> topologicalSort(
        const std::vector<std::string>& nodes,
        const std::vector<std::pair<std::string, std::string>>& edges);
};

} // namespace gef

#endif
