#include <gef/app.h>
#include <print>

auto main() -> int {
    gef::ModuleRegistry module_registry;
    gef::Context ctx;

    std::println("Basic component instantiation test passed!");
    return 0;
}
