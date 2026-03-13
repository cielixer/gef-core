#ifndef GEF_MODULEVARIANT_H_
#define GEF_MODULEVARIANT_H_

#include <gef/core/binding/Common.h>
#include <gef/core/binding/Context.h>
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

/// C++ internal metadata for all module kinds.
/// AtomicModules convert gef_metadata_t -> ModuleSignature on load;
/// FlowModules / PipelineModules build their own at composition time.
struct ModuleSignature {
    std::string          version;
    std::vector<Binding> bindings;
};

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

/// Directed edge in a FlowModule DAG: "instance.binding" format.
struct FlowEdge {
    std::string from;
    std::string to;
};

using ModuleHandle = void*;
using gef_execute_fn_t = void (*)(gef::Context&);

struct AtomicModule {
    ModuleHandle          handle;
    const gef_metadata_t* metadata; // lifetime tied to .so
    gef_execute_fn_t      execute;
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
    std::string     name; // qualified: "flow1.subflow2.adder1"
    ModuleSignature signature;
    ModuleVariant   variant;
};

} // namespace gef

#endif
