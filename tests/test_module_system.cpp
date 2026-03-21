#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <filesystem>
#include <gef/app.h>
#include <gef/core/module/ModuleVariant.h>
#include <stdexcept>
#include "TestModulePath.h"

namespace fs = std::filesystem;

static gef::ModuleDef makeAtomicModuleDef(std::string name,
                                          std::string version = "0.1.0") {
    return gef::ModuleDef{
        .name = std::move(name),
        .signature = gef::ModuleSignature{
            .version = std::move(version),
            .bindings = {},
        },
        .variant = gef::createAtomicModule(nullptr, nullptr, nullptr),
    };
}

TEST_CASE("AtomicModuleRegistry can load example_add module", "[module]") {
    gef::ModuleRegistry registry;
    auto module_path = getModulePath("example_add");

    auto name = gef::loadAtomicModule(registry, module_path);
    REQUIRE(name.has_value());
    REQUIRE(*name == "example_add");

    auto atomic = gef::getAtomicModule(registry, "example_add");
    REQUIRE(atomic.has_value());
    const auto* metadata = gef::moduleMetadata(**atomic);
    REQUIRE(metadata != nullptr);
    REQUIRE(std::string(metadata->module_name) == "example_add");
    REQUIRE(metadata->num_bindings == 3);

    REQUIRE(std::string(metadata->bindings[0].name) == "lhs");
    REQUIRE(std::string(metadata->bindings[1].name) == "rhs");
    REQUIRE(std::string(metadata->bindings[2].name) == "result");
}

TEST_CASE("AtomicModuleRegistry can execute example_add module", "[module][execute]") {
    gef::ModuleRegistry registry;
    auto module_path = getModulePath("example_add");

    auto name = gef::loadAtomicModule(registry, module_path);
    REQUIRE(name.has_value());

    auto atomic = gef::getAtomicModule(registry, *name);
    REQUIRE(atomic.has_value());

    gef::Context ctx;

    int a_value      = 2;
    int b_value      = 3;
    int result_value = 0;

    ctx.set_binding("lhs", std::any(&a_value));
    ctx.set_binding("rhs", std::any(&b_value));
    ctx.set_binding("result", std::any(&result_value));

    gef::executeAtomicModule(**atomic, ctx);

    REQUIRE(result_value == 5);
}

TEST_CASE("AtomicModuleRegistry tracks loaded module names", "[module]") {
    gef::ModuleRegistry registry;

    auto add_name = gef::loadAtomicModule(registry, getModulePath("example_add"));
    auto sub_name = gef::loadAtomicModule(registry, getModulePath("example_subtract"));

    REQUIRE(add_name.has_value());
    REQUIRE(sub_name.has_value());

    auto names = gef::atomicModuleNames(registry);
    REQUIRE(names.size() == 2);
    REQUIRE((std::find(names.begin(), names.end(), "example_add") != names.end()));
    REQUIRE((std::find(names.begin(), names.end(), "example_subtract") != names.end()));
}

TEST_CASE("System loads and executes example_add end-to-end", "[system][execute]") {
    gef::System system;
    auto module_path = getModulePath("example_add");

    auto id = system.loadModule(module_path);
    REQUIRE(id.has_value());

    gef::Context ctx;

    int a_value      = 10;
    int b_value      = 20;
    int result_value = 0;

    ctx.set_binding("lhs", std::any(&a_value));
    ctx.set_binding("rhs", std::any(&b_value));
    ctx.set_binding("result", std::any(&result_value));

    auto exec_result = system.executeModule("example_add", ctx);
    REQUIRE(exec_result.has_value());

    REQUIRE(result_value == 30);
}

TEST_CASE("System hot-reloads existing module without duplicate registration", "[system][hot-reload]") {
    gef::System system;
    auto module_path = getModulePath("example_add");

    auto first_id = system.loadModule(module_path);
    REQUIRE(first_id.has_value());

    auto second_id = system.loadModule(module_path);
    REQUIRE(second_id.has_value());

    REQUIRE(*first_id == *second_id);
    REQUIRE(system.moduleRegistry().size() == 1);
    REQUIRE(gef::atomicModuleNames(system.moduleRegistry()).size() == 1);

    gef::Context ctx;
    int a_value = 8;
    int b_value = 13;
    int result_value = 0;

    ctx.set_binding("lhs", std::any(&a_value));
    ctx.set_binding("rhs", std::any(&b_value));
    ctx.set_binding("result", std::any(&result_value));

    auto exec_result = system.executeModule(*second_id, ctx);
    REQUIRE(exec_result.has_value());
    REQUIRE(result_value == 21);
}

TEST_CASE("System handles missing modules gracefully", "[module][error]") {
    gef::System system;

    auto result = system.loadModule(getModulePath("nonexistent"));
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code == gef::ErrorCode::FileNotFound);
}

TEST_CASE("AtomicModuleRegistry handles missing module name gracefully", "[module][error]") {
    gef::ModuleRegistry registry;

    auto result = gef::getAtomicModule(registry, "nonexistent_module");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code == gef::ErrorCode::ModuleNotFound);
}

TEST_CASE("System executeModule by ModuleId works", "[system][execute]") {
    gef::System system;
    auto module_path = getModulePath("example_add");

    auto id = system.loadModule(module_path);
    REQUIRE(id.has_value());

    gef::Context ctx;

    int a_value      = 5;
    int b_value      = 7;
    int result_value = 0;

    ctx.set_binding("lhs", std::any(&a_value));
    ctx.set_binding("rhs", std::any(&b_value));
    ctx.set_binding("result", std::any(&result_value));

    auto exec_result = system.executeModule(*id, ctx);
    REQUIRE(exec_result.has_value());

    REQUIRE(result_value == 12);
}

TEST_CASE("ModuleRegistry stores ModuleDef with correct signature", "[registry]") {
    gef::System system;
    auto module_path = getModulePath("example_add");

    auto id = system.loadModule(module_path);
    REQUIRE(id.has_value());

    auto def = system.moduleRegistry().get(*id);
    REQUIRE(def.has_value());
    REQUIRE((*def)->name == "example_add");
    REQUIRE((*def)->signature.version == "0.1.0");
    REQUIRE((*def)->signature.bindings.size() == 3);
    REQUIRE((*def)->signature.bindings[0].name == "lhs");
    REQUIRE((*def)->signature.bindings[0].role == gef::BindingRole::Input);
    REQUIRE((*def)->signature.bindings[2].name == "result");
    REQUIRE((*def)->signature.bindings[2].role == gef::BindingRole::Output);
}

TEST_CASE("AtomicModuleRegistry rejects empty module path", "[module][error]") {
    gef::ModuleRegistry registry;
    REQUIRE_THROWS_AS((void)gef::loadAtomicModule(registry, fs::path{}),
                      std::invalid_argument);
}

TEST_CASE("AtomicModuleRegistry rejects non-file module path", "[module][error]") {
    gef::ModuleRegistry registry;
    auto result = gef::loadAtomicModule(registry, fs::path(GEF_MODULE_DIR));

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code == gef::ErrorCode::InvalidFileType);
}

TEST_CASE("System executeModule rejects empty module name", "[system][error]") {
    gef::System system;
    gef::Context ctx;

    REQUIRE_THROWS_AS((void)system.executeModule("", ctx), std::invalid_argument);
}

TEST_CASE("System executeModule by id returns error for unknown id", "[system][error]") {
    gef::System system;
    gef::Context ctx;

    auto result = system.executeModule(static_cast<gef::ModuleId>(12345), ctx);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code == gef::ErrorCode::ModuleNotFound);
}

TEST_CASE("System executeModule by name returns error for unknown module", "[system][error]") {
    gef::System system;
    gef::Context ctx;

    auto result = system.executeModule("missing.module", ctx);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code == gef::ErrorCode::ModuleNotFound);
}

TEST_CASE("System executeModule handles non-atomic variants", "[system][variant]") {
    gef::System system;
    auto& registry = system.moduleRegistry();

    auto flow_id = registry.add(gef::ModuleDef{
        .name = "flow.placeholder",
        .signature = gef::ModuleSignature{"0.1.0", {}},
        .variant = gef::FlowModule{{}, {}},
    });

    auto pipeline_id = registry.add(gef::ModuleDef{
        .name = "pipeline.placeholder",
        .signature = gef::ModuleSignature{"0.1.0", {}},
        .variant = gef::PipelineModule{{}},
    });

    gef::Context ctx;

    auto flow_result = system.executeModule(flow_id, ctx);
    REQUIRE(flow_result.has_value());

    auto pipeline_result = system.executeModule(pipeline_id, ctx);
    REQUIRE(pipeline_result.has_value());
}

TEST_CASE("ModuleRegistry add/find/get success path", "[registry]") {
    gef::ModuleRegistry registry;

    auto id = registry.add(makeAtomicModuleDef("unit.example"));
    REQUIRE(id == 0);
    REQUIRE(registry.size() == 1);

    auto found_id = registry.find("unit.example");
    REQUIRE(found_id.has_value());
    REQUIRE(*found_id == id);

    auto def = registry.get(id);
    REQUIRE(def.has_value());
    REQUIRE((*def)->name == "unit.example");
    REQUIRE((*def)->signature.version == "0.1.0");
}

TEST_CASE("ModuleRegistry validates add inputs", "[registry][error]") {
    gef::ModuleRegistry registry;

    REQUIRE_THROWS_AS(registry.add(makeAtomicModuleDef("")), std::invalid_argument);

    auto first_id = registry.add(makeAtomicModuleDef("duplicate.name"));
    REQUIRE(first_id == 0);
    REQUIRE_THROWS_AS(registry.add(makeAtomicModuleDef("duplicate.name")),
                      std::invalid_argument);
}

TEST_CASE("ModuleRegistry reports missing entries", "[registry][error]") {
    gef::ModuleRegistry registry;

    auto missing_id = registry.find("missing.module");
    REQUIRE_FALSE(missing_id.has_value());
    REQUIRE(missing_id.error().code == gef::ErrorCode::ModuleNotFound);

    auto missing_def = registry.get(static_cast<gef::ModuleId>(0));
    REQUIRE_FALSE(missing_def.has_value());
    REQUIRE(missing_def.error().code == gef::ErrorCode::ModuleNotFound);
}

TEST_CASE("Context typed accessors expose mutable and const references", "[context]") {
    gef::Context ctx;

    int input_value = 4;
    int output_value = 0;
    int inout_value = 10;
    int config_value = 99;

    ctx.set_binding("in", std::any(&input_value));
    ctx.set_binding("out", std::any(&output_value));
    ctx.set_binding("io", std::any(&inout_value));
    ctx.set_binding("cfg", std::any(&config_value));

    REQUIRE(ctx.input<int>("in") == 4);

    auto& out_ref = ctx.output<int>("out");
    out_ref = 12;
    REQUIRE(output_value == 12);

    auto& inout_ref = ctx.inout<int>("io");
    inout_ref += 5;
    REQUIRE(inout_value == 15);

    REQUIRE(ctx.config<int>("cfg") == 99);
}

TEST_CASE("Context throws bad_any_cast for missing or mismatched bindings", "[context][error]") {
    gef::Context ctx;

    REQUIRE_THROWS_AS(ctx.input<int>("missing"), std::bad_any_cast);

    float value = 3.5f;
    ctx.set_binding("number", std::any(&value));
    REQUIRE_THROWS_AS(ctx.input<int>("number"), std::bad_any_cast);
}

TEST_CASE("Module path helper resolves existing built module", "[module][path]") {
    auto path = getModulePath("example_add");
    REQUIRE(fs::exists(path));
    REQUIRE(fs::is_regular_file(path));
}

TEST_CASE("LoadAtomicModule detects missing gef_get_metadata", "[module][error][symbol]") {
    gef::ModuleRegistry registry;
    auto path = getModulePath("missing_metadata");
    
    auto result = gef::loadAtomicModule(registry, path);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code == gef::ErrorCode::SymbolNotFound);
}

TEST_CASE("LoadAtomicModule detects missing gef_execute", "[module][error][symbol]") {
    gef::ModuleRegistry registry;
    auto path = getModulePath("missing_execute");
    
    auto result = gef::loadAtomicModule(registry, path);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code == gef::ErrorCode::SymbolNotFound);
}

TEST_CASE("LoadAtomicModule detects null metadata", "[module][error][metadata]") {
    gef::ModuleRegistry registry;
    auto path = getModulePath("null_metadata");
    
    auto result = gef::loadAtomicModule(registry, path);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code == gef::ErrorCode::MetadataInvalid);
}
