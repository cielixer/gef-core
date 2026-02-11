#include <gef/Macros.h>

GEF_MODULE("example_add", "0.1.0",
    GEF_INPUT(int, "a"),
    GEF_INPUT(int, "b"),
    GEF_OUTPUT(int, "sum")
)
{
    auto a   = ctx.input<int>("a");
    auto b   = ctx.input<int>("b");
    auto sum = ctx.output<int>("sum");

    *sum = *a + *b;
}
