#include <gef/core/scheduler/Scheduler.h>
#include <map>
#include <set>
#include <stdexcept>

namespace gef {

std::vector<std::string> Scheduler::topologicalSort(
    const std::vector<std::string>& nodes,
    const std::vector<std::pair<std::string, std::string>>& edges) {
  
  std::vector<std::string> result;
  
  if (nodes.empty()) {
    return result;
  }

  // Build adjacency list and in-degree map
  std::map<std::string, std::vector<std::string>> adjacency;
  std::map<std::string, int> in_degree;
  
  // Initialize in-degree for all nodes
  for (const auto& node : nodes) {
    in_degree[node] = 0;
    adjacency[node];  // Ensure node exists in adjacency
  }
  
  // Process edges and build adjacency list
  for (const auto& [source, destination] : edges) {
    adjacency[source].push_back(destination);
    in_degree[destination]++;
  }
  
  // Queue: nodes with in-degree 0, maintained in sorted order for determinism
  std::set<std::string> queue;
  for (const auto& [node, degree] : in_degree) {
    if (degree == 0) {
      queue.insert(node);
    }
  }
  
  // Process queue (Kahn's algorithm with lexicographic tie-breaking)
  while (!queue.empty()) {
    // Dequeue lexicographically smallest node
    auto current = *queue.begin();
    queue.erase(queue.begin());
    result.push_back(current);
    
    // Process neighbors
    for (const auto& neighbor : adjacency[current]) {
      in_degree[neighbor]--;
      if (in_degree[neighbor] == 0) {
        queue.insert(neighbor);
      }
    }
  }
  
  // Cycle detection: if not all nodes were processed, a cycle exists
  if (result.size() != nodes.size()) [[unlikely]] {
    throw std::runtime_error("Cycle detected in module graph");
  }
  
  return result;
}

} // namespace gef
