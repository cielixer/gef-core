#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <gef/app.h>
#include <gef/core/scheduler/Scheduler.h>

namespace fs = std::filesystem;

static fs::path getModulePath(const std::string& name) {
    return fs::path(GEF_MODULE_DIR) / ("lib" + name + ".so");
}

TEST_CASE("Scheduler handles linear chain A->B->C", "[flow][scheduler]") {
    std::vector<std::string> nodes = {"A", "B", "C"};
    std::vector<std::pair<std::string, std::string>> edges = {
        {"A", "B"},
        {"B", "C"}
    };
    
    auto result = gef::Scheduler::topologicalSort(nodes, edges);
    
    REQUIRE(result.size() == 3);
    REQUIRE(result[0] == "A");
    REQUIRE(result[1] == "B");
    REQUIRE(result[2] == "C");
}

TEST_CASE("Scheduler handles diamond graph with deterministic tie-breaking", "[flow][scheduler]") {
    std::vector<std::string> nodes = {"A", "B", "C", "D"};
    std::vector<std::pair<std::string, std::string>> edges = {
        {"A", "B"},
        {"A", "C"},
        {"B", "D"},
        {"C", "D"}
    };
    
    auto result = gef::Scheduler::topologicalSort(nodes, edges);
    
    REQUIRE(result.size() == 4);
    REQUIRE(result[0] == "A");
    REQUIRE((result[1] == "B" || result[1] == "C"));
    REQUIRE((result[2] == "B" || result[2] == "C"));
    REQUIRE(result[3] == "D");
}

TEST_CASE("Scheduler handles single node with no edges", "[flow][scheduler]") {
    std::vector<std::string> nodes = {"A"};
    std::vector<std::pair<std::string, std::string>> edges = {};
    
    auto result = gef::Scheduler::topologicalSort(nodes, edges);
    
    REQUIRE(result.size() == 1);
    REQUIRE(result[0] == "A");
}

TEST_CASE("Scheduler handles empty graph", "[flow][scheduler]") {
    std::vector<std::string> nodes = {};
    std::vector<std::pair<std::string, std::string>> edges = {};
    
    auto result = gef::Scheduler::topologicalSort(nodes, edges);
    
    REQUIRE(result.empty());
}

TEST_CASE("Scheduler detects cycle and throws", "[flow][scheduler][error]") {
    std::vector<std::string> nodes = {"A", "B"};
    std::vector<std::pair<std::string, std::string>> edges = {
        {"A", "B"},
        {"B", "A"}
    };
    
    REQUIRE_THROWS_AS(
        gef::Scheduler::topologicalSort(nodes, edges),
        std::runtime_error
    );
}

TEST_CASE("Scheduler orders independent nodes lexicographically", "[flow][scheduler]") {
    std::vector<std::string> nodes = {"C", "A", "B"};
    std::vector<std::pair<std::string, std::string>> edges = {};

    auto result = gef::Scheduler::topologicalSort(nodes, edges);

    REQUIRE(result == std::vector<std::string>{"A", "B", "C"});
}

TEST_CASE("Scheduler throws when edges reference unknown nodes", "[flow][scheduler][error]") {
    std::vector<std::string> nodes = {"A", "B"};
    std::vector<std::pair<std::string, std::string>> edges = {
        {"A", "C"}
    };

    REQUIRE_THROWS_AS(
        gef::Scheduler::topologicalSort(nodes, edges),
        std::runtime_error
    );
}

TEST_CASE("Flow addModule accepts valid module name", "[flow][addModule]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    system.loadModule(module_path).value();

    gef::Flow flow(system.moduleRegistry());
    
    REQUIRE_NOTHROW(flow.addModule("adder1", "example_add"));
}

TEST_CASE("Flow addModule rejects duplicate instance name", "[flow][addModule][error]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    system.loadModule(module_path).value();

    gef::Flow flow(system.moduleRegistry());
    flow.addModule("adder1", "example_add");
    
    REQUIRE_THROWS_AS(
        flow.addModule("adder1", "example_add"),
        std::runtime_error
    );
}

TEST_CASE("Flow addModule rejects unknown module type", "[flow][addModule][error]") {
    gef::System system;
    gef::Flow flow(system.moduleRegistry());
    
    REQUIRE_THROWS_AS(
        flow.addModule("unknown_inst", "nonexistent_module"),
        std::runtime_error
    );
}

TEST_CASE("Flow connect accepts valid endpoints", "[flow][connect]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    system.loadModule(module_path).value();

    gef::Flow flow(system.moduleRegistry());
    flow.addModule("adder1", "example_add");
    
    REQUIRE_NOTHROW(flow.connect<int>("inputs.lhs", "adder1.lhs"));
    REQUIRE_NOTHROW(flow.connect<int>("adder1.result", "outputs.result"));
}

TEST_CASE("Flow connect rejects unknown instance in from endpoint", "[flow][connect][error]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    system.loadModule(module_path).value();

    gef::Flow flow(system.moduleRegistry());
    flow.addModule("adder1", "example_add");
    
    REQUIRE_THROWS_AS(
        flow.connect<int>("unknown.output", "adder1.lhs"),
        std::runtime_error
    );
}

TEST_CASE("Flow connect rejects unknown instance in to endpoint", "[flow][connect][error]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    system.loadModule(module_path).value();

    gef::Flow flow(system.moduleRegistry());
    flow.addModule("adder1", "example_add");
    
    REQUIRE_THROWS_AS(
        flow.connect<int>("adder1.result", "unknown.input"),
        std::runtime_error
    );
}

TEST_CASE("Flow connect rejects malformed from endpoint", "[flow][connect][error]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    system.loadModule(module_path).value();

    gef::Flow flow(system.moduleRegistry());
    flow.addModule("adder1", "example_add");

    REQUIRE_THROWS_AS(
        flow.connect<int>("malformed", "adder1.lhs"),
        std::runtime_error
    );
}

TEST_CASE("Flow connect rejects malformed to endpoint", "[flow][connect][error]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    system.loadModule(module_path).value();

    gef::Flow flow(system.moduleRegistry());
    flow.addModule("adder1", "example_add");

    REQUIRE_THROWS_AS(
        flow.connect<int>("adder1.result", "malformed"),
        std::runtime_error
    );
}


TEST_CASE("Flow executes linear chain with two adders", "[flow][execute][integration]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    system.loadModule(module_path).value();
    
    gef::Flow flow(system.moduleRegistry());
    flow.addModule("adder1", "example_add");
    flow.addModule("adder2", "example_add");
    flow.connect<int>("inputs.lhs", "adder1.lhs");
    flow.connect<int>("inputs.rhs", "adder1.rhs");
    flow.connect<int>("adder1.result", "adder2.lhs");
    flow.connect<int>("inputs.offset", "adder2.rhs");
    flow.connect<int>("adder2.result", "outputs.sum");
    
    gef::Context ctx;
    int lhs = 2, rhs = 3, offset = 10, sum = 0;
    ctx.set_binding("lhs", std::any(&lhs));
    ctx.set_binding("rhs", std::any(&rhs));
    ctx.set_binding("offset", std::any(&offset));
    ctx.set_binding("sum", std::any(&sum));
    
    flow.execute(ctx);
    
    REQUIRE(sum == 15);
}

TEST_CASE("Flow boundary inputs/outputs work correctly", "[flow][execute][boundary]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    system.loadModule(module_path).value();
    
    gef::Flow flow(system.moduleRegistry());
    flow.addModule("adder", "example_add");
    flow.connect<int>("inputs.a", "adder.lhs");
    flow.connect<int>("inputs.b", "adder.rhs");
    flow.connect<int>("adder.result", "outputs.c");
    
    gef::Context ctx;
    int a = 7, b = 8, c = 0;
    ctx.set_binding("a", std::any(&a));
    ctx.set_binding("b", std::any(&b));
    ctx.set_binding("c", std::any(&c));
    
    flow.execute(ctx);
    
    REQUIRE(c == 15);
}

TEST_CASE("Flow detects cycles and throws", "[flow][execute][error]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    system.loadModule(module_path).value();
    
    gef::Flow flow(system.moduleRegistry());
    flow.addModule("adder1", "example_add");
    flow.addModule("adder2", "example_add");
    flow.connect<int>("adder1.result", "adder2.lhs");
    flow.connect<int>("adder2.result", "adder1.rhs");
    flow.connect<int>("inputs.seed", "adder2.rhs");
    
    gef::Context ctx;
    int seed = 1;
    ctx.set_binding("seed", std::any(&seed));
    
    REQUIRE_THROWS_AS(flow.execute(ctx), std::runtime_error);
}

TEST_CASE("Flow config injection works", "[flow][execute][config]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    system.loadModule(module_path).value();
    
    gef::Flow flow(system.moduleRegistry());
    flow.addModule("adder", "example_add");
    flow.connect<int>("inputs.a", "adder.lhs");
    flow.connect<int>("inputs.b", "adder.rhs");
    flow.connect<int>("adder.result", "outputs.result");
    
    int multiplier = 2;
    flow.setConfig<int>("multiplier", multiplier);
    
    gef::Context ctx;
    int a = 5, b = 3, result = 0;
    ctx.set_binding("a", std::any(&a));
    ctx.set_binding("b", std::any(&b));
    ctx.set_binding("result", std::any(&result));
    
    flow.execute(ctx);
    
    REQUIRE(result == 8);
}

TEST_CASE("Flow executes diamond DAG correctly", "[flow][execute][integration]") {
    gef::System system;
    system.loadModule(getModulePath("example_add")).value();
    
    gef::Flow flow(system.moduleRegistry());
    flow.addModule("left", "example_add");
    flow.addModule("right", "example_add");
    flow.addModule("merger", "example_add");
    
    flow.connect<int>("inputs.value", "left.lhs");
    flow.connect<int>("inputs.left_offset", "left.rhs");
    flow.connect<int>("inputs.value", "right.lhs");
    flow.connect<int>("inputs.right_offset", "right.rhs");
    flow.connect<int>("left.result", "merger.lhs");
    flow.connect<int>("right.result", "merger.rhs");
    flow.connect<int>("merger.result", "outputs.final");
    
    gef::Context ctx;
    int value = 10, left_offset = 5, right_offset = 3, final = 0;
    ctx.set_binding("value", std::any(&value));
    ctx.set_binding("left_offset", std::any(&left_offset));
    ctx.set_binding("right_offset", std::any(&right_offset));
    ctx.set_binding("final", std::any(&final));
    
    flow.execute(ctx);
    
    REQUIRE(final == 28);
}

TEST_CASE("Flow handles multi-output modules", "[flow][execute][integration]") {
    gef::System system;
    system.loadModule(getModulePath("example_divide")).value();
    system.loadModule(getModulePath("example_add")).value();
    
    gef::Flow flow(system.moduleRegistry());
    flow.addModule("divider", "example_divide");
    flow.addModule("adder", "example_add");
    
    flow.connect<int>("inputs.dividend", "divider.lhs");
    flow.connect<int>("inputs.divisor", "divider.rhs");
    flow.connect<int>("divider.quotient", "adder.lhs");
    flow.connect<int>("divider.remainder", "adder.rhs");
    flow.connect<int>("adder.result", "outputs.sum");
    
    gef::Context ctx;
    int dividend = 17, divisor = 5, sum = 0;
    ctx.set_binding("dividend", std::any(&dividend));
    ctx.set_binding("divisor", std::any(&divisor));
    ctx.set_binding("sum", std::any(&sum));
    
    flow.execute(ctx);
    
    REQUIRE(sum == 5);
}

TEST_CASE("Flow chains multiple instances of same module", "[flow][execute][integration]") {
    gef::System system;
    system.loadModule(getModulePath("example_add")).value();
    
    gef::Flow flow(system.moduleRegistry());
    flow.addModule("step1", "example_add");
    flow.addModule("step2", "example_add");
    flow.addModule("step3", "example_add");
    
    flow.connect<int>("inputs.initial", "step1.lhs");
    flow.connect<int>("inputs.increment1", "step1.rhs");
    flow.connect<int>("step1.result", "step2.lhs");
    flow.connect<int>("inputs.increment2", "step2.rhs");
    flow.connect<int>("step2.result", "step3.lhs");
    flow.connect<int>("inputs.increment3", "step3.rhs");
    flow.connect<int>("step3.result", "outputs.final");
    
    gef::Context ctx;
    int initial = 1, increment1 = 10, increment2 = 100, increment3 = 1000, final = 0;
    ctx.set_binding("initial", std::any(&initial));
    ctx.set_binding("increment1", std::any(&increment1));
    ctx.set_binding("increment2", std::any(&increment2));
    ctx.set_binding("increment3", std::any(&increment3));
    ctx.set_binding("final", std::any(&final));
    
    flow.execute(ctx);
    
    REQUIRE(final == 1111);
}
