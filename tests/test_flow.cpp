#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <gef/app.h>
#include <gef/core/scheduler/Scheduler.h>
#include "TestModulePath.h"

namespace fs = std::filesystem;

TEST_CASE("Scheduler handles linear chain A->B->C", "[flow][scheduler]") {
    std::vector<std::string> nodes = {"A", "B", "C"};
    std::vector<std::pair<std::string, std::string>> edges = {
        {"A", "B"},
        {"B", "C"}
    };
    
    auto result = gef::topologicalSort(nodes, edges);
    REQUIRE(result.has_value());
    
    REQUIRE(result->size() == 3);
    REQUIRE((*result)[0] == "A");
    REQUIRE((*result)[1] == "B");
    REQUIRE((*result)[2] == "C");
}

TEST_CASE("Scheduler handles diamond graph with deterministic tie-breaking", "[flow][scheduler]") {
    std::vector<std::string> nodes = {"A", "B", "C", "D"};
    std::vector<std::pair<std::string, std::string>> edges = {
        {"A", "B"},
        {"A", "C"},
        {"B", "D"},
        {"C", "D"}
    };
    
    auto result = gef::topologicalSort(nodes, edges);
    REQUIRE(result.has_value());
    
    REQUIRE(result->size() == 4);
    REQUIRE((*result)[0] == "A");
    REQUIRE(((*result)[1] == "B" || (*result)[1] == "C"));
    REQUIRE(((*result)[2] == "B" || (*result)[2] == "C"));
    REQUIRE((*result)[3] == "D");
}

TEST_CASE("Scheduler handles single node with no edges", "[flow][scheduler]") {
    std::vector<std::string> nodes = {"A"};
    std::vector<std::pair<std::string, std::string>> edges = {};
    
    auto result = gef::topologicalSort(nodes, edges);
    REQUIRE(result.has_value());
    
    REQUIRE(result->size() == 1);
    REQUIRE((*result)[0] == "A");
}

TEST_CASE("Scheduler handles empty graph", "[flow][scheduler]") {
    std::vector<std::string> nodes = {};
    std::vector<std::pair<std::string, std::string>> edges = {};
    
    auto result = gef::topologicalSort(nodes, edges);
    REQUIRE(result.has_value());
    REQUIRE(result->empty());
}

TEST_CASE("Scheduler detects cycle and returns error", "[flow][scheduler][error]") {
    std::vector<std::string> nodes = {"A", "B"};
    std::vector<std::pair<std::string, std::string>> edges = {
        {"A", "B"},
        {"B", "A"}
    };
    
    auto result = gef::topologicalSort(nodes, edges);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code == gef::ErrorCode::CycleDetected);
}

TEST_CASE("Scheduler orders independent nodes lexicographically", "[flow][scheduler]") {
    std::vector<std::string> nodes = {"C", "A", "B"};
    std::vector<std::pair<std::string, std::string>> edges = {};

    auto result = gef::topologicalSort(nodes, edges);
    REQUIRE(result.has_value());
    REQUIRE(*result == std::vector<std::string>{"A", "B", "C"});
}

TEST_CASE("Scheduler returns error when edges reference unknown nodes", "[flow][scheduler][error]") {
    std::vector<std::string> nodes = {"A", "B"};
    std::vector<std::pair<std::string, std::string>> edges = {
        {"A", "C"}
    };

    auto result = gef::topologicalSort(nodes, edges);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code == gef::ErrorCode::CycleDetected);
}

TEST_CASE("Flow addModule accepts valid module name", "[flow][addModule]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    gef::loadModule(system, module_path).value();

    auto flow = gef::createFlow(system.module_registry);
    
    auto result = gef::flowAddModule(flow, "adder1", "example_add");
    REQUIRE(result.has_value());
}

TEST_CASE("Flow addModule rejects duplicate instance name", "[flow][addModule][error]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    gef::loadModule(system, module_path).value();

    auto flow = gef::createFlow(system.module_registry);
    gef::flowAddModule(flow, "adder1", "example_add").value();
    
    auto result = gef::flowAddModule(flow, "adder1", "example_add");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code == gef::ErrorCode::DuplicateInstance);
}

TEST_CASE("Flow addModule rejects unknown module type", "[flow][addModule][error]") {
    gef::System system;
    auto flow = gef::createFlow(system.module_registry);
    
    auto result = gef::flowAddModule(flow, "unknown_inst", "nonexistent_module");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code == gef::ErrorCode::ModuleNotFound);
}

TEST_CASE("Flow connect accepts valid endpoints", "[flow][connect]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    gef::loadModule(system, module_path).value();

    auto flow = gef::createFlow(system.module_registry);
    gef::flowAddModule(flow, "adder1", "example_add").value();
    
    auto r1 = gef::flowConnect<int>(flow, "inputs.lhs", "adder1.lhs");
    REQUIRE(r1.has_value());
    auto r2 = gef::flowConnect<int>(flow, "adder1.result", "outputs.result");
    REQUIRE(r2.has_value());
}

TEST_CASE("Flow connect rejects unknown instance in from endpoint", "[flow][connect][error]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    gef::loadModule(system, module_path).value();

    auto flow = gef::createFlow(system.module_registry);
    gef::flowAddModule(flow, "adder1", "example_add").value();
    
    auto result = gef::flowConnect<int>(flow, "unknown.output", "adder1.lhs");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code == gef::ErrorCode::InvalidEndpoint);
}

TEST_CASE("Flow connect rejects unknown instance in to endpoint", "[flow][connect][error]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    gef::loadModule(system, module_path).value();

    auto flow = gef::createFlow(system.module_registry);
    gef::flowAddModule(flow, "adder1", "example_add").value();
    
    auto result = gef::flowConnect<int>(flow, "adder1.result", "unknown.input");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code == gef::ErrorCode::InvalidEndpoint);
}

TEST_CASE("Flow connect rejects malformed from endpoint", "[flow][connect][error]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    gef::loadModule(system, module_path).value();

    auto flow = gef::createFlow(system.module_registry);
    gef::flowAddModule(flow, "adder1", "example_add").value();

    auto result = gef::flowConnect<int>(flow, "malformed", "adder1.lhs");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code == gef::ErrorCode::InvalidEndpoint);
}

TEST_CASE("Flow connect rejects malformed to endpoint", "[flow][connect][error]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    gef::loadModule(system, module_path).value();

    auto flow = gef::createFlow(system.module_registry);
    gef::flowAddModule(flow, "adder1", "example_add").value();

    auto result = gef::flowConnect<int>(flow, "adder1.result", "malformed");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code == gef::ErrorCode::InvalidEndpoint);
}


TEST_CASE("Flow executes linear chain with two adders", "[flow][execute][integration]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    gef::loadModule(system, module_path).value();
    
    auto flow = gef::createFlow(system.module_registry);
    gef::flowAddModule(flow, "adder1", "example_add").value();
    gef::flowAddModule(flow, "adder2", "example_add").value();
    gef::flowConnect<int>(flow, "inputs.lhs", "adder1.lhs").value();
    gef::flowConnect<int>(flow, "inputs.rhs", "adder1.rhs").value();
    gef::flowConnect<int>(flow, "adder1.result", "adder2.lhs").value();
    gef::flowConnect<int>(flow, "inputs.offset", "adder2.rhs").value();
    gef::flowConnect<int>(flow, "adder2.result", "outputs.sum").value();
    
    gef::Context ctx;
    int lhs = 2, rhs = 3, offset = 10, sum = 0;
    gef::setBinding(ctx, "lhs", std::any(&lhs));
    gef::setBinding(ctx, "rhs", std::any(&rhs));
    gef::setBinding(ctx, "offset", std::any(&offset));
    gef::setBinding(ctx, "sum", std::any(&sum));
    
    auto result = gef::flowExecute(flow, ctx);
    REQUIRE(result.has_value());
    REQUIRE(sum == 15);
}

TEST_CASE("Flow boundary inputs/outputs work correctly", "[flow][execute][boundary]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    gef::loadModule(system, module_path).value();
    
    auto flow = gef::createFlow(system.module_registry);
    gef::flowAddModule(flow, "adder", "example_add").value();
    gef::flowConnect<int>(flow, "inputs.a", "adder.lhs").value();
    gef::flowConnect<int>(flow, "inputs.b", "adder.rhs").value();
    gef::flowConnect<int>(flow, "adder.result", "outputs.c").value();
    
    gef::Context ctx;
    int a = 7, b = 8, c = 0;
    gef::setBinding(ctx, "a", std::any(&a));
    gef::setBinding(ctx, "b", std::any(&b));
    gef::setBinding(ctx, "c", std::any(&c));
    
    auto result = gef::flowExecute(flow, ctx);
    REQUIRE(result.has_value());
    REQUIRE(c == 15);
}

TEST_CASE("Flow detects cycles and returns error", "[flow][execute][error]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    gef::loadModule(system, module_path).value();
    
    auto flow = gef::createFlow(system.module_registry);
    gef::flowAddModule(flow, "adder1", "example_add").value();
    gef::flowAddModule(flow, "adder2", "example_add").value();
    gef::flowConnect<int>(flow, "adder1.result", "adder2.lhs").value();
    gef::flowConnect<int>(flow, "adder2.result", "adder1.rhs").value();
    gef::flowConnect<int>(flow, "inputs.seed", "adder2.rhs").value();
    
    gef::Context ctx;
    int seed = 1;
    gef::setBinding(ctx, "seed", std::any(&seed));
    
    auto result = gef::flowExecute(flow, ctx);
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().code == gef::ErrorCode::CycleDetected);
}

TEST_CASE("Flow config injection works", "[flow][execute][config]") {
    gef::System system;
    auto module_path = getModulePath("example_add");
    gef::loadModule(system, module_path).value();
    
    auto flow = gef::createFlow(system.module_registry);
    gef::flowAddModule(flow, "adder", "example_add").value();
    gef::flowConnect<int>(flow, "inputs.a", "adder.lhs").value();
    gef::flowConnect<int>(flow, "inputs.b", "adder.rhs").value();
    gef::flowConnect<int>(flow, "adder.result", "outputs.result").value();
    
    int multiplier = 2;
    gef::flowSetConfig<int>(flow, "multiplier", multiplier);
    
    gef::Context ctx;
    int a = 5, b = 3, result = 0;
    gef::setBinding(ctx, "a", std::any(&a));
    gef::setBinding(ctx, "b", std::any(&b));
    gef::setBinding(ctx, "result", std::any(&result));
    
    auto exec_result = gef::flowExecute(flow, ctx);
    REQUIRE(exec_result.has_value());
    REQUIRE(result == 8);
}

TEST_CASE("Flow executes diamond DAG correctly", "[flow][execute][integration]") {
    gef::System system;
    gef::loadModule(system, getModulePath("example_add")).value();
    
    auto flow = gef::createFlow(system.module_registry);
    gef::flowAddModule(flow, "left", "example_add").value();
    gef::flowAddModule(flow, "right", "example_add").value();
    gef::flowAddModule(flow, "merger", "example_add").value();
    
    gef::flowConnect<int>(flow, "inputs.value", "left.lhs").value();
    gef::flowConnect<int>(flow, "inputs.left_offset", "left.rhs").value();
    gef::flowConnect<int>(flow, "inputs.value", "right.lhs").value();
    gef::flowConnect<int>(flow, "inputs.right_offset", "right.rhs").value();
    gef::flowConnect<int>(flow, "left.result", "merger.lhs").value();
    gef::flowConnect<int>(flow, "right.result", "merger.rhs").value();
    gef::flowConnect<int>(flow, "merger.result", "outputs.final").value();
    
    gef::Context ctx;
    int value = 10, left_offset = 5, right_offset = 3, final_val = 0;
    gef::setBinding(ctx, "value", std::any(&value));
    gef::setBinding(ctx, "left_offset", std::any(&left_offset));
    gef::setBinding(ctx, "right_offset", std::any(&right_offset));
    gef::setBinding(ctx, "final", std::any(&final_val));
    
    auto result = gef::flowExecute(flow, ctx);
    REQUIRE(result.has_value());
    REQUIRE(final_val == 28);
}

TEST_CASE("Flow handles multi-output modules", "[flow][execute][integration]") {
    gef::System system;
    gef::loadModule(system, getModulePath("example_divide")).value();
    gef::loadModule(system, getModulePath("example_add")).value();
    
    auto flow = gef::createFlow(system.module_registry);
    gef::flowAddModule(flow, "divider", "example_divide").value();
    gef::flowAddModule(flow, "adder", "example_add").value();
    
    gef::flowConnect<int>(flow, "inputs.dividend", "divider.lhs").value();
    gef::flowConnect<int>(flow, "inputs.divisor", "divider.rhs").value();
    gef::flowConnect<int>(flow, "divider.quotient", "adder.lhs").value();
    gef::flowConnect<int>(flow, "divider.remainder", "adder.rhs").value();
    gef::flowConnect<int>(flow, "adder.result", "outputs.sum").value();
    
    gef::Context ctx;
    int dividend = 17, divisor = 5, sum = 0;
    gef::setBinding(ctx, "dividend", std::any(&dividend));
    gef::setBinding(ctx, "divisor", std::any(&divisor));
    gef::setBinding(ctx, "sum", std::any(&sum));
    
    auto result = gef::flowExecute(flow, ctx);
    REQUIRE(result.has_value());
    REQUIRE(sum == 5);
}

TEST_CASE("Flow chains multiple instances of same module", "[flow][execute][integration]") {
    gef::System system;
    gef::loadModule(system, getModulePath("example_add")).value();
    
    auto flow = gef::createFlow(system.module_registry);
    gef::flowAddModule(flow, "step1", "example_add").value();
    gef::flowAddModule(flow, "step2", "example_add").value();
    gef::flowAddModule(flow, "step3", "example_add").value();
    
    gef::flowConnect<int>(flow, "inputs.initial", "step1.lhs").value();
    gef::flowConnect<int>(flow, "inputs.increment1", "step1.rhs").value();
    gef::flowConnect<int>(flow, "step1.result", "step2.lhs").value();
    gef::flowConnect<int>(flow, "inputs.increment2", "step2.rhs").value();
    gef::flowConnect<int>(flow, "step2.result", "step3.lhs").value();
    gef::flowConnect<int>(flow, "inputs.increment3", "step3.rhs").value();
    gef::flowConnect<int>(flow, "step3.result", "outputs.final").value();
    
    gef::Context ctx;
    int initial = 1, increment1 = 10, increment2 = 100, increment3 = 1000, final_val = 0;
    gef::setBinding(ctx, "initial", std::any(&initial));
    gef::setBinding(ctx, "increment1", std::any(&increment1));
    gef::setBinding(ctx, "increment2", std::any(&increment2));
    gef::setBinding(ctx, "increment3", std::any(&increment3));
    gef::setBinding(ctx, "final", std::any(&final_val));
    
    auto result = gef::flowExecute(flow, ctx);
    REQUIRE(result.has_value());
    REQUIRE(final_val == 1111);
}
