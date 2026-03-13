#include <gef/module.hpp>

void execute(gef::Context& ctx) {
    auto& lhs   = ctx.input<int>("lhs");
    auto& rhs   = ctx.input<int>("rhs");
    auto& quotient = ctx.output<int>("quotient");
    auto& remainder = ctx.output<int>("remainder");

    quotient = lhs / rhs;
    remainder = lhs % rhs;
}

// clang-format off
GEF_MODULE("0.1.0", execute,
    GEF_INPUT(int, "lhs"),
    GEF_INPUT(int, "rhs"),
    GEF_OUTPUT(int, "quotient"),
    GEF_OUTPUT(int, "remainder")
)
// clang-format on
