#include <gef/module.hpp>

void execute(gef::Context& ctx) {
    auto& lhs    = gef::ctxInput<int>(ctx, "lhs");
    auto& rhs    = gef::ctxInput<int>(ctx, "rhs");
    auto& result = gef::ctxOutput<int>(ctx, "result");

    result = lhs * rhs;
}

// clang-format off
GEF_MODULE("0.1.0", execute,
    GEF_INPUT(int, "lhs"),
    GEF_INPUT(int, "rhs"),
    GEF_OUTPUT(int, "result")
)
// clang-format on
