#ifndef GEF_MODULEVARIANT_H_
#define GEF_MODULEVARIANT_H_

#include "gef/core/binding/Common.h"
#include "gef/core/binding/Context.h"
#include "gef/core/module/AtomicModule.h"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace gef {

using ModuleId = std::uint32_t;

inline constexpr ModuleId kInvalidModuleId = static_cast<ModuleId>(-1);

enum class BindingRole { Input, Output, InOut, Config };

struct Binding {
    std::string name;
    BindingRole role;
    std::string type_name;
};

struct ModuleSignature {
    std::string          version;
    std::vector<Binding> bindings;
};

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

struct FlowEdge {
    std::string from;
    std::string to;
};

struct FlowModule {
    std::vector<ModuleId> children;
    std::vector<FlowEdge> edges;
};

struct PipelineModule {
    std::vector<ModuleId> stages;
};

using ModuleVariant = std::variant<AtomicModule, FlowModule, PipelineModule>;

struct ModuleDef {
    std::string     name;
    ModuleSignature signature;
    ModuleVariant   variant;
};

} // namespace gef

#endif
