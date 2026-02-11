#include <catch2/catch_test_macros.hpp>
#include <gef/PluginLoader.h>
#include <gef/Registry.h>
#include <gef/System.h>
#include <gef/Context.h>
#include <cstdlib>
#include <filesystem>
#include <iostream>

// Helper to get the path to the built module
static std::string getModulePath() {
    // Try different possible paths: absolute, relative to build dir, or relative to project
    std::filesystem::path candidates[] = {
        std::filesystem::path(std::getenv("CMAKE_BINARY_DIR") ? std::getenv("CMAKE_BINARY_DIR") : "") / "modules" / "libexample_add.so",
        "/Volumes/media/Workspace/dev/gef/build/modules/libexample_add.so",
        "modules/libexample_add.so",
        "../modules/libexample_add.so",
    };
    
    for (const auto& path : candidates) {
        if (path.string().empty()) continue;
        if (std::filesystem::exists(path)) {
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
    auto get_metadata = reinterpret_cast<const gef_metadata_t* (*)()>(
        loader.getSymbol(handle, "gef_get_metadata")
    );
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

TEST_CASE("Plugin system can execute example_add module", "[plugin][execute]") {
    gef::PluginLoader loader;
    std::string module_path = getModulePath();
    
    void* handle = loader.load(module_path.c_str());
    REQUIRE(handle != nullptr);
    
    auto get_metadata = reinterpret_cast<const gef_metadata_t* (*)()>(
        loader.getSymbol(handle, "gef_get_metadata")
    );
    REQUIRE(get_metadata != nullptr);
    
    gef::Context ctx;
    
    int a_value = 2;
    int b_value = 3;
    int sum_result = 0;
    
    ctx.set_binding("a", std::any(&a_value));
    ctx.set_binding("b", std::any(&b_value));
    ctx.set_binding("sum", std::any(&sum_result));
    
    const gef_metadata_t* metadata = get_metadata();
    REQUIRE(metadata != nullptr);
    REQUIRE(std::string(metadata->module_name) == "example_add");
    REQUIRE(metadata->num_bindings == 3);
    
    // Verify that the context can store and retrieve the bindings
    int* a_ptr = std::any_cast<int*>(ctx.get_binding("a"));
    int* b_ptr = std::any_cast<int*>(ctx.get_binding("b"));
    int* sum_ptr = std::any_cast<int*>(ctx.get_binding("sum"));
    
    REQUIRE(a_ptr != nullptr);
    REQUIRE(b_ptr != nullptr);
    REQUIRE(sum_ptr != nullptr);
    REQUIRE(*a_ptr == 2);
    REQUIRE(*b_ptr == 3);
    
    loader.unload(handle);
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
