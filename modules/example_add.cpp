#include <gef/Macros.h>

void execute(gef::Context& ctx) {
    auto& a   = ctx.input<int>("a");
    auto& b   = ctx.input<int>("b");
    auto& sum = ctx.output<int>("sum");

    sum = a + b;
}

GEF_MODULE("example_add", "0.1.0", execute,
    GEF_INPUT(int, "a"),
    GEF_INPUT(int, "b"),
    GEF_OUTPUT(int, "sum")
)
