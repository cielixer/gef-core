#include <gef/module.hpp>

void execute(gef::Context& ctx) {
    auto& lhs   = ctx.input<int>("lhs");
    auto& rhs   = ctx.input<int>("rhs");
    auto& result = ctx.output<int>("result");

    result = lhs * rhs;
}

// clang-format off
GEF_MODULE("0.1.0", execute,
    GEF_INPUT(int, "lhs"),
    GEF_INPUT(int, "rhs"),
    GEF_OUTPUT(int, "result")
)
// clang-format on
