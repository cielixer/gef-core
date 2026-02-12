#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <filesystem>
#include <gef/Context.h>
#include <gef/Module.h>
#include <gef/PluginLoader.h>
#include <gef/Registry.h>
#include <gef/System.h>

namespace fs = std::filesystem;

// Helper to get the path to the built module
static std::string getModulePath() {
    // Try different possible paths: absolute, relative to build dir, or relative to project
    fs::path candidates[] = {
        fs::path(std::getenv("CMAKE_BINARY_DIR") ? std::getenv("CMAKE_BINARY_DIR") : "") /
            "modules" / "libexample_add.so",
        "/Volumes/media/Workspace/dev/gef/build/modules/libexample_add.so",
        "modules/libexample_add.so",
        "../modules/libexample_add.so",
    };

    for (const auto& path : candidates) {
        if (path.string().empty())
            continue;
        if (fs::exists(path)) {
            return path.string();
        }
    }

    // Return the most likely path (absolute path in build dir)
    return "/Volumes/media/Workspace/dev/gef/build/modules/libexample_add.so";
}

TEST_CASE("Plugin system can load example_add module", "[plugin]") {
    gef::PluginLoader loader;
    std::string module_path = getModulePath();

    void* handle = loader.load(module_path.c_str());
    REQUIRE(handle != nullptr);

    // Load the gef_get_metadata symbol
    auto get_metadata =
        reinterpret_cast<const gef_metadata_t* (*)()>(loader.getSymbol(handle, "gef_get_metadata"));
    REQUIRE(get_metadata != nullptr);

    // Call it to get metadata
    const gef_metadata_t* metadata = get_metadata();
    REQUIRE(metadata != nullptr);

    // Verify metadata
    REQUIRE(std::string(metadata->module_name) == "example_add");
    REQUIRE(metadata->num_bindings == 3);

    // Verify binding names and types
    REQUIRE(std::string(metadata->bindings[0].name) == "a");
    REQUIRE(std::string(metadata->bindings[1].name) == "b");
    REQUIRE(std::string(metadata->bindings[2].name) == "sum");

    loader.unload(handle);
}

TEST_CASE("Plugin system can execute example_add module via dlsym", "[plugin][execute]") {
    gef::PluginLoader loader;
    std::string module_path = getModulePath();

    void* handle = loader.load(module_path.c_str());
    REQUIRE(handle != nullptr);

    auto get_metadata =
        reinterpret_cast<const gef_metadata_t* (*)()>(loader.getSymbol(handle, "gef_get_metadata"));
    REQUIRE(get_metadata != nullptr);

    auto execute = reinterpret_cast<gef_execute_fn>(loader.getSymbol(handle, "gef_execute"));
    REQUIRE(execute != nullptr);

    gef::Context ctx;

    int a_value    = 2;
    int b_value    = 3;
    int sum_result = 0;

    ctx.set_binding("a", std::any(&a_value));
    ctx.set_binding("b", std::any(&b_value));
    ctx.set_binding("sum", std::any(&sum_result));

    execute(ctx);

    REQUIRE(sum_result == 5);

    loader.unload(handle);
}

TEST_CASE("System loads and executes example_add end-to-end", "[system][execute]") {
    gef::System system;
    std::string module_path = getModulePath();

    system.loadModule(module_path.c_str());

    gef::Context ctx;

    int a_value    = 10;
    int b_value    = 20;
    int sum_result = 0;

    ctx.set_binding("a", std::any(&a_value));
    ctx.set_binding("b", std::any(&b_value));
    ctx.set_binding("sum", std::any(&sum_result));

    system.executeModule("example_add", ctx);

    REQUIRE(sum_result == 30);
}

TEST_CASE("Plugin system handles missing modules gracefully", "[plugin][error]") {
    gef::System system;

    // Attempting to load a non-existent module should throw
    REQUIRE_THROWS(system.loadModule("nonexistent_module.so"));
}

TEST_CASE("Plugin system handles missing symbols gracefully", "[plugin][error]") {
    gef::PluginLoader loader;
    std::string module_path = getModulePath();

    void* handle = loader.load(module_path.c_str());
    REQUIRE(handle != nullptr);

    // Trying to get a non-existent symbol should throw an exception
    REQUIRE_THROWS(loader.getSymbol(handle, "nonexistent_symbol_12345"));

    loader.unload(handle);
}
