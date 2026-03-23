#ifndef GEF_SCHEDULER_H_
#define GEF_SCHEDULER_H_

#include "gef/core/system/Error.h"

#include <expected>
#include <string>
#include <utility>
#include <vector>

namespace gef {

[[nodiscard]] auto topologicalSort(
    const std::vector<std::string>& nodes,
    const std::vector<std::pair<std::string, std::string>>& edges)
    -> std::expected<std::vector<std::string>, Error>;

} // namespace gef

#endif
