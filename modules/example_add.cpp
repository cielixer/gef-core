#include <gef/Module.h>
#include <gef/Context.h>
#include <gef/Macros.h>

class ExampleAdd : public gef::AtomicModule {
public:
    void execute(gef::Context& ctx) override {
        auto a = ctx.input<int>("a");
        auto b = ctx.input<int>("b");
        auto sum = ctx.output<int>("sum");
        
        *sum = *a + *b;
    }
};

static constexpr gef_binding_t _gef_bindings[] = {
    GEF_INPUT(int, "a"),
    GEF_INPUT(int, "b"),
    GEF_OUTPUT(int, "sum")
};

static const gef_metadata_t _gef_metadata = {
    .module_name = "example_add",
    .version = "0.1.0",
    .bindings = _gef_bindings,
    .num_bindings = 3
};

extern "C" const gef_metadata_t* gef_get_metadata(void) {
    return &_gef_metadata;
}
