#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <filesystem>
#include <gef/app.h>
#include <gef/core/module/Module.h>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

namespace fs = std::filesystem;

static fs::path getModulePath(const std::string& name) {
    return fs::path(GEF_MODULE_DIR) / ("lib" + name + ".so");
}

static gef::ModuleId loadAndRegister(gef::ModuleStore& store,
                                     const fs::path& path) {
    auto atomic = gef::loadAtomicModule(path);
    REQUIRE(atomic.has_value());

    gef::ModuleSignature sig;
    sig.version = atomic->metadata->version;
    for (std::size_t i = 0; i < atomic->metadata->num_bindings; ++i) {
        const auto& b = atomic->metadata->bindings[i];
        sig.bindings.push_back(gef::Binding{
            b.name,
            static_cast<gef::BindingRole>(b.role),
            b.type_name,
        });
    }

    std::string name{atomic->metadata->module_name};
    gef::ModuleDef def{
        .name      = std::move(name),
        .signature = std::move(sig),
        .variant   = std::move(*atomic),
    };

    return gef::registerModule(store, std::move(def));
}

static gef::ModuleDef makeAtomicModuleDef(std::string name,
                                          std::string version = "0.1.0") {
    return gef::ModuleDef{
        .name = std::move(name),
        .signature = gef::ModuleSignature{
            .version = std::move(version),
            .bindings = {},
        },
        .variant = gef::AtomicModule{},
    };
}

TEST_CASE("ModuleStore can load example_add module", "[module]") {
    gef::ModuleStore store;
    auto module_path = getModulePath("example_add");

    loadAndRegister(store, module_path);

    auto id = gef::findModule(store, "example_add");
    REQUIRE(id.has_value());

    auto def = gef::getModule(store, *id);
    REQUIRE(def.has_value());

    auto* atomic = std::get_if<gef::AtomicModule>(&(*def)->variant);
    REQUIRE(atomic != nullptr);
    REQUIRE(atomic->metadata != nullptr);
    REQUIRE(std::string(atomic->metadata->module_name) == "example_add");
    REQUIRE(atomic->metadata->num_bindings == 3);

    REQUIRE(std::string(atomic->metadata->bindings[0].name) == "lhs");
    REQUIRE(std::string(atomic->metadata->bindings[1].name) == "rhs");
    REQUIRE(std::string(atomic->metadata->bindings[2].name) == "result");
}

TEST_CASE("ModuleStore can execute example_add module", "[module][execute]") {
    gef::ModuleStore store;
    auto module_path = getModulePath("example_add");

    loadAndRegister(store, module_path);

    auto id = gef::findModule(store, "example_add");
    REQUIRE(id.has_value());

    auto def = gef::getModule(store, *id);
    REQUIRE(def.has_value());

    auto* atomic = std::get_if<gef::AtomicModule>(&(*def)->variant);
    REQUIRE(atomic != nullptr);

    gef::Context ctx;

    int a_value      = 2;
    int b_value      = 3;
    int result_value = 0;

    ctx.set_binding("lhs", std::any(&a_value));
    ctx.set_binding("rhs", std::any(&b_value));
    ctx.set_binding("result", std::any(&result_value));

    atomic->execute(ctx);

    REQUIRE(result_value == 5);
}

TEST_CASE("ModuleStore tracks loaded module names", "[module]") {
    gef::ModuleStore store;

    loadAndRegister(store, getModulePath("example_add"));
    loadAndRegister(store, getModulePath("example_subtract"));

    REQUIRE(gef::findModule(store, "example_add").has_value());
    REQUIRE(gef::findModule(store, "example_subtract").has_value());
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

TEST_CASE("System rejects duplicate module load when hot-reload is disabled", "[system][hot-reload]") {
    gef::System system;
    auto module_path = getModulePath("example_add");

    auto first_id = system.loadModule(module_path);
    REQUIRE(first_id.has_value());

    auto second_id = system.loadModule(module_path);
    REQUIRE_FALSE(second_id.has_value());
    REQUIRE(second_id.error().code == gef::ErrorCode::LoadFailed);

    REQUIRE(gef::findModule(system.moduleStore(), "example_add").has_value());

    gef::Context ctx;
    int a_value = 8;
    int b_value = 13;
    int result_value = 0;

    ctx.set_binding("lhs", std::any(&a_value));
    ctx.set_binding("rhs", std::any(&b_value));
    ctx.set_binding("result", std::any(&result_value));

    auto exec_result = system.executeModule(*first_id, ctx);
    REQUIRE(exec_result.has_value());
    REQUIRE(result_value == 21);
}

TEST_CASE("System handles missing modules gracefully", "[module][error]") {
    gef::System system;

    auto result = system.loadModule("nonexistent_module.so");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code == gef::ErrorCode::FileNotFound);
}

TEST_CASE("ModuleStore findModule handles missing module name gracefully", "[module][error]") {
    gef::ModuleStore store;

    auto result = gef::findModule(store, "nonexistent_module");
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

TEST_CASE("ModuleStore stores ModuleDef with correct signature", "[store]") {
    gef::System system;
    auto module_path = getModulePath("example_add");

    auto id = system.loadModule(module_path);
    REQUIRE(id.has_value());

    auto def = gef::getModule(system.moduleStore(), *id);
    REQUIRE(def.has_value());
    REQUIRE((*def)->name == "example_add");
    REQUIRE((*def)->signature.version == "0.1.0");
    REQUIRE((*def)->signature.bindings.size() == 3);
    REQUIRE((*def)->signature.bindings[0].name == "lhs");
    REQUIRE((*def)->signature.bindings[0].role == gef::BindingRole::Input);
    REQUIRE((*def)->signature.bindings[2].name == "result");
    REQUIRE((*def)->signature.bindings[2].role == gef::BindingRole::Output);
}

TEST_CASE("loadAtomicModule rejects empty module path", "[module][error]") {
    REQUIRE_THROWS_AS((void)gef::loadAtomicModule(fs::path{}),
                      std::invalid_argument);
}

TEST_CASE("loadAtomicModule rejects non-file module path", "[module][error]") {
    auto result = gef::loadAtomicModule(fs::path(GEF_MODULE_DIR));

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
    auto& store = system.moduleStore();

    auto composite_id = gef::registerModule(store, gef::ModuleDef{
        .name = "composite.placeholder",
        .signature = gef::ModuleSignature{"0.1.0", {}},
        .variant = gef::CompositeModule{},
    });

    gef::Context ctx;

    auto id_result = system.executeModule(composite_id, ctx);
    REQUIRE(id_result.has_value());

    auto name_result = system.executeModule("composite.placeholder", ctx);
    REQUIRE(name_result.has_value());
}

TEST_CASE("ModuleStore registerModule/findModule/getModule success path", "[store]") {
    gef::ModuleStore store;

    auto id = gef::registerModule(store, makeAtomicModuleDef("unit.example"));
    REQUIRE(id == 0);

    auto found_id = gef::findModule(store, "unit.example");
    REQUIRE(found_id.has_value());
    REQUIRE(*found_id == id);

    auto def = gef::getModule(store, id);
    REQUIRE(def.has_value());
    REQUIRE((*def)->name == "unit.example");
    REQUIRE((*def)->signature.version == "0.1.0");
}

TEST_CASE("ModuleStore getModule exposes immutable ModuleDef pointers", "[store][thread]") {
    using GetModuleReturn = decltype(gef::getModule(
        std::declval<const gef::ModuleStore&>(), std::declval<gef::ModuleId>()));
    static_assert(std::is_same_v<GetModuleReturn,
                                 std::expected<const gef::ModuleDef*, gef::Error>>);

    gef::ModuleStore store;
    auto id = gef::registerModule(store, makeAtomicModuleDef("threadsafe.const-view"));

    auto def = gef::getModule(store, id);
    REQUIRE(def.has_value());
    REQUIRE((*def)->name == "threadsafe.const-view");
}

TEST_CASE("ModuleStore supports concurrent register and read", "[store][thread]") {
    gef::ModuleStore store;

    constexpr int reader_iterations = 5000;
    constexpr int writer_count = 128;

    std::atomic<int> observed_reads{0};

    auto reader = [&store, &observed_reads]() {
        for (int i = 0; i < reader_iterations; ++i) {
            auto id = gef::findModule(store, "threadsafe.module.0");
            if (!id) {
                std::this_thread::yield();
                continue;
            }

            auto def = gef::getModule(store, *id);
            if (def && (*def)->name == "threadsafe.module.0") {
                observed_reads.fetch_add(1, std::memory_order_relaxed);
            }
        }
    };

    std::thread reader_a(reader);
    std::thread reader_b(reader);

    for (int i = 0; i < writer_count; ++i) {
        auto name = std::string("threadsafe.module.") + std::to_string(i);
        auto id = gef::registerModule(store, makeAtomicModuleDef(name));
        REQUIRE(id == static_cast<gef::ModuleId>(i));
    }

    reader_a.join();
    reader_b.join();

    REQUIRE(observed_reads.load(std::memory_order_relaxed) > 0);

    for (int i = 0; i < writer_count; ++i) {
        auto name = std::string("threadsafe.module.") + std::to_string(i);
        auto id = gef::findModule(store, name);
        REQUIRE(id.has_value());

        auto def = gef::getModule(store, *id);
        REQUIRE(def.has_value());
        REQUIRE((*def)->name == name);
    }
}

TEST_CASE("ModuleDef pointer remains stable across concurrent append-only registration", "[store][thread]") {
    gef::ModuleStore store;
    auto seed_id = gef::registerModule(store, makeAtomicModuleDef("stable.seed"));

    auto seed_def = gef::getModule(store, seed_id);
    REQUIRE(seed_def.has_value());

    const gef::ModuleDef* ptr = *seed_def;
    REQUIRE(ptr != nullptr);
    REQUIRE(ptr->name == "stable.seed");

    constexpr int extra_modules = 256;

    std::thread writer([&store]() {
        for (int i = 0; i < extra_modules; ++i) {
            auto name = std::string("stable.extra.") + std::to_string(i);
            (void)gef::registerModule(store, makeAtomicModuleDef(name));
        }
    });

    for (int i = 0; i < 10000; ++i) {
        REQUIRE(ptr->name == "stable.seed");
    }

    writer.join();

    REQUIRE(ptr->name == "stable.seed");
    auto last_extra_name = std::string("stable.extra.") + std::to_string(extra_modules - 1);
    REQUIRE(gef::findModule(store, last_extra_name).has_value());
}

TEST_CASE("System concurrent duplicate load yields one success and one LoadFailed", "[system][thread]") {
    gef::System system;
    auto module_path = getModulePath("example_add");

    std::atomic<int> success_count{0};
    std::atomic<int> load_failed_count{0};
    std::atomic<int> other_error_count{0};
    std::atomic<bool> start{false};

    auto worker = [&]() {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }

        auto result = system.loadModule(module_path);
        if (result) {
            success_count.fetch_add(1, std::memory_order_relaxed);
            return;
        }

        if (result.error().code == gef::ErrorCode::LoadFailed) {
            load_failed_count.fetch_add(1, std::memory_order_relaxed);
            return;
        }

        other_error_count.fetch_add(1, std::memory_order_relaxed);
    };

    std::thread t1(worker);
    std::thread t2(worker);

    start.store(true, std::memory_order_release);

    t1.join();
    t2.join();

    REQUIRE(success_count.load(std::memory_order_relaxed) == 1);
    REQUIRE(load_failed_count.load(std::memory_order_relaxed) == 1);
    REQUIRE(other_error_count.load(std::memory_order_relaxed) == 0);
    REQUIRE(gef::findModule(system.moduleStore(), "example_add").has_value());
}

TEST_CASE("ModuleStore validates registerModule inputs", "[store][error]") {
    gef::ModuleStore store;

    REQUIRE_THROWS_AS(gef::registerModule(store, makeAtomicModuleDef("")),
                      std::invalid_argument);

    auto first_id = gef::registerModule(store, makeAtomicModuleDef("duplicate.name"));
    REQUIRE(first_id == 0);
    REQUIRE_THROWS_AS(gef::registerModule(store, makeAtomicModuleDef("duplicate.name")),
                      std::invalid_argument);
}

TEST_CASE("ModuleStore reports missing entries", "[store][error]") {
    gef::ModuleStore store;

    auto missing_id = gef::findModule(store, "missing.module");
    REQUIRE_FALSE(missing_id.has_value());
    REQUIRE(missing_id.error().code == gef::ErrorCode::ModuleNotFound);

    auto missing_def = gef::getModule(store, static_cast<gef::ModuleId>(0));
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
