#include <gef/module.hpp>

void execute(gef::Context& ctx) {
    auto& lhs       = gef::ctxInput<int>(ctx, "lhs");
    auto& rhs       = gef::ctxInput<int>(ctx, "rhs");
    auto& quotient  = gef::ctxOutput<int>(ctx, "quotient");
    auto& remainder = gef::ctxOutput<int>(ctx, "remainder");

    quotient  = lhs / rhs;
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
