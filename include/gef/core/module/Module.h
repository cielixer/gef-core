#ifndef GEF_MODULE_H_
#define GEF_MODULE_H_

#include <gef/core/module/AtomicModule.h>
#include <gef/core/module/CompositeModule.h>
#include <gef/core/binding/Common.h>
#include <gef/core/binding/Context.h>
#include <string>
#include <variant>
#include <vector>

namespace gef {

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

using ModuleVariant = std::variant<AtomicModule, CompositeModule>;

struct ModuleDef {
    std::string     name;
    ModuleSignature signature;
    ModuleVariant   variant;
};

}

typedef void (*gef_execute_fn)(gef::Context& ctx);

#ifdef __cplusplus
extern "C" {
#endif

extern const gef_metadata_t* gef_get_metadata(void);
extern void gef_execute(gef::Context& ctx);

#ifdef __cplusplus
}
#endif

#endif
