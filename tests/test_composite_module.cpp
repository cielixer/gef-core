#include <catch2/catch_test_macros.hpp>
#include <gef/core/module/CompositeModule.h>
#include <gef/core/module/ModuleStore.h>
#include <gef/core/system/Error.h>

TEST_CASE("CompositeChildId type exists", "[composite]") {
    gef::CompositeChildId id = 0;
    REQUIRE(id == 0);
}

TEST_CASE("kInvalidCompositeChildId constant defined", "[composite]") {
    REQUIRE(gef::kInvalidCompositeChildId == static_cast<gef::CompositeChildId>(-1));
}

TEST_CASE("CompositeChild struct can be constructed", "[composite]") {
    gef::CompositeChild child{"inst_name", 42};
    REQUIRE(child.instance_name == "inst_name");
    REQUIRE(child.module_id == 42);
}

TEST_CASE("CompositeEdge struct can be constructed", "[composite]") {
    gef::CompositeEdge edge{0, 1};
    REQUIRE(edge.from == 0);
    REQUIRE(edge.to == 1);
}

TEST_CASE("CompositeModule struct can be empty", "[composite]") {
    gef::CompositeModule empty{};
    REQUIRE(empty.children.empty());
    REQUIRE(empty.edges.empty());
}

TEST_CASE("CompositeModule can hold children", "[composite]") {
    gef::CompositeModule cm;
    cm.children.push_back({"inst1", 10});
    cm.children.push_back({"inst2", 20});

    REQUIRE(cm.children.size() == 2);
    REQUIRE(cm.children[0].instance_name == "inst1");
    REQUIRE(cm.children[0].module_id == 10);
    REQUIRE(cm.children[1].instance_name == "inst2");
    REQUIRE(cm.children[1].module_id == 20);
}

TEST_CASE("CompositeModule can hold edges", "[composite]") {
    gef::CompositeModule cm;
    cm.edges.push_back({0, 1});
    cm.edges.push_back({1, 2});

    REQUIRE(cm.edges.size() == 2);
    REQUIRE(cm.edges[0].from == 0);
    REQUIRE(cm.edges[0].to == 1);
    REQUIRE(cm.edges[1].from == 1);
    REQUIRE(cm.edges[1].to == 2);
}

TEST_CASE("CompositeModule is a value type", "[composite]") {
    gef::CompositeModule cm1;
    cm1.children.push_back({"inst", 5});
    cm1.edges.push_back({0, 1});

    gef::CompositeModule cm2 = cm1;
    REQUIRE(cm2.children.size() == 1);
    REQUIRE(cm2.edges.size() == 1);
    REQUIRE(cm2.children[0].instance_name == "inst");

    cm2.children.clear();
    REQUIRE(cm1.children.size() == 1);
}

TEST_CASE("CompositeModule addChild and findChild use local child ids", "[composite]") {
    gef::CompositeModule cm;

    auto result1 = gef::addChild(cm, "inst1", 10);
    REQUIRE(result1.has_value());
    REQUIRE(result1.value() == 0);

    auto result2 = gef::addChild(cm, "inst2", 20);
    REQUIRE(result2.has_value());
    REQUIRE(result2.value() == 1);

    auto result3 = gef::addChild(cm, "inst3", 30);
    REQUIRE(result3.has_value());
    REQUIRE(result3.value() == 2);

    REQUIRE(cm.children.size() == 3);
    REQUIRE(cm.children[0].instance_name == "inst1");
    REQUIRE(cm.children[1].instance_name == "inst2");
    REQUIRE(cm.children[2].instance_name == "inst3");

    auto find1 = gef::findChild(cm, "inst1");
    REQUIRE(find1.has_value());
    REQUIRE(find1.value() == 0);

    auto find2 = gef::findChild(cm, "inst2");
    REQUIRE(find2.has_value());
    REQUIRE(find2.value() == 1);

    auto find3 = gef::findChild(cm, "inst3");
    REQUIRE(find3.has_value());
    REQUIRE(find3.value() == 2);
}

TEST_CASE("CompositeModule addChild allows same module_id under distinct names", "[composite]") {
    gef::CompositeModule cm;

    auto r1 = gef::addChild(cm, "alias1", 42);
    REQUIRE(r1.has_value());
    REQUIRE(r1.value() == 0);

    auto r2 = gef::addChild(cm, "alias2", 42);
    REQUIRE(r2.has_value());
    REQUIRE(r2.value() == 1);

    auto r3 = gef::addChild(cm, "alias3", 42);
    REQUIRE(r3.has_value());
    REQUIRE(r3.value() == 2);

    REQUIRE(cm.children.size() == 3);
    REQUIRE(cm.children[0].module_id == 42);
    REQUIRE(cm.children[1].module_id == 42);
    REQUIRE(cm.children[2].module_id == 42);

    auto find_alias1 = gef::findChild(cm, "alias1");
    REQUIRE(find_alias1.has_value());
    REQUIRE(find_alias1.value() == 0);

    auto find_alias2 = gef::findChild(cm, "alias2");
    REQUIRE(find_alias2.has_value());
    REQUIRE(find_alias2.value() == 1);
}

TEST_CASE("CompositeModule addChild rejects empty names and invalid module ids", "[composite]") {
    gef::CompositeModule cm;

    auto empty_name = gef::addChild(cm, "", 10);
    REQUIRE(!empty_name.has_value());
    REQUIRE(empty_name.error().code == gef::ErrorCode::InvalidTopology);
    REQUIRE(empty_name.error().message.find("empty") != std::string::npos);

    auto invalid_id = gef::addChild(cm, "inst", gef::kInvalidModuleId);
    REQUIRE(!invalid_id.has_value());
    REQUIRE(invalid_id.error().code == gef::ErrorCode::InvalidTopology);
    REQUIRE(invalid_id.error().message.find("kInvalidModuleId") != std::string::npos);

    REQUIRE(cm.children.empty());
}

TEST_CASE("CompositeModule findChild returns not-found error for missing names", "[composite]") {
    gef::CompositeModule cm;

    auto r1 = gef::addChild(cm, "inst1", 10);
    REQUIRE(r1.has_value());

    auto r2 = gef::addChild(cm, "inst2", 20);
    REQUIRE(r2.has_value());

    auto missing = gef::findChild(cm, "inst3");
    REQUIRE(!missing.has_value());
    REQUIRE(missing.error().code == gef::ErrorCode::ModuleNotFound);
    REQUIRE(missing.error().message.find("inst3") != std::string::npos);

    auto also_missing = gef::findChild(cm, "nonexistent");
    REQUIRE(!also_missing.has_value());
    REQUIRE(also_missing.error().code == gef::ErrorCode::ModuleNotFound);
}

TEST_CASE("CompositeModule children are appended in insertion order", "[composite]") {
    gef::CompositeModule cm;

    std::vector<std::string> names = {"first", "second", "third", "fourth"};
    for (size_t i = 0; i < names.size(); ++i) {
        auto result = gef::addChild(cm, names[i], 100 + static_cast<gef::ModuleId>(i));
        REQUIRE(result.has_value());
        REQUIRE(result.value() == static_cast<gef::CompositeChildId>(i));
    }

    REQUIRE(cm.children.size() == 4);
    for (size_t i = 0; i < names.size(); ++i) {
        REQUIRE(cm.children[i].instance_name == names[i]);
        REQUIRE(cm.children[i].module_id == 100 + static_cast<gef::ModuleId>(i));
    }
}

TEST_CASE("CompositeModule addEdge rejects invalid or duplicate edges", "[composite]") {
    gef::CompositeModule cm;
    
    auto id0 = gef::addChild(cm, "child0", 10);
    auto id1 = gef::addChild(cm, "child1", 20);
    auto id2 = gef::addChild(cm, "child2", 30);
    REQUIRE(id0.has_value());
    REQUIRE(id1.has_value());
    REQUIRE(id2.has_value());

    // Valid edge succeeds
    auto valid = gef::addEdge(cm, 0, 1);
    REQUIRE(valid.has_value());
    REQUIRE(cm.edges.size() == 1);
    REQUIRE(cm.edges[0].from == 0);
    REQUIRE(cm.edges[0].to == 1);

    // Duplicate edge rejected
    auto duplicate = gef::addEdge(cm, 0, 1);
    REQUIRE(!duplicate.has_value());
    REQUIRE(duplicate.error().code == gef::ErrorCode::InvalidTopology);
    REQUIRE(duplicate.error().message.find("Duplicate") != std::string::npos);
    REQUIRE(cm.edges.size() == 1);

    // Self-edge rejected
    auto self_edge = gef::addEdge(cm, 1, 1);
    REQUIRE(!self_edge.has_value());
    REQUIRE(self_edge.error().code == gef::ErrorCode::InvalidTopology);
    REQUIRE(self_edge.error().message.find("Self-edge") != std::string::npos);
    REQUIRE(cm.edges.size() == 1);

    // Out-of-range source rejected
    auto bad_source = gef::addEdge(cm, 99, 1);
    REQUIRE(!bad_source.has_value());
    REQUIRE(bad_source.error().code == gef::ErrorCode::InvalidTopology);
    REQUIRE(bad_source.error().message.find("out of range") != std::string::npos);
    REQUIRE(cm.edges.size() == 1);

    // Out-of-range target rejected
    auto bad_target = gef::addEdge(cm, 1, 99);
    REQUIRE(!bad_target.has_value());
    REQUIRE(bad_target.error().code == gef::ErrorCode::InvalidTopology);
    REQUIRE(bad_target.error().message.find("out of range") != std::string::npos);
    REQUIRE(cm.edges.size() == 1);

    // Multiple distinct edges succeed
    auto edge2 = gef::addEdge(cm, 1, 2);
    REQUIRE(edge2.has_value());
    REQUIRE(cm.edges.size() == 2);

    auto edge3 = gef::addEdge(cm, 0, 2);
    REQUIRE(edge3.has_value());
    REQUIRE(cm.edges.size() == 3);
}

TEST_CASE("CompositeModule topologicalOrder handles empty composite", "[composite]") {
    gef::CompositeModule cm;
    
    auto order = gef::topologicalOrder(cm);
    REQUIRE(order.has_value());
    REQUIRE(order.value().empty());
}

TEST_CASE("CompositeModule topologicalOrder handles single node", "[composite]") {
    gef::CompositeModule cm;
    gef::addChild(cm, "only", 10);
    
    auto order = gef::topologicalOrder(cm);
    REQUIRE(order.has_value());
    REQUIRE(order.value().size() == 1);
    REQUIRE(order.value()[0] == 0);
}

TEST_CASE("CompositeModule topologicalOrder handles linear DAG", "[composite]") {
    gef::CompositeModule cm;
    
    auto id0 = gef::addChild(cm, "first", 10);
    auto id1 = gef::addChild(cm, "second", 20);
    auto id2 = gef::addChild(cm, "third", 30);
    REQUIRE(id0.has_value());
    REQUIRE(id1.has_value());
    REQUIRE(id2.has_value());

    gef::addEdge(cm, 0, 1);
    gef::addEdge(cm, 1, 2);

    auto order = gef::topologicalOrder(cm);
    REQUIRE(order.has_value());
    REQUIRE(order.value().size() == 3);
    REQUIRE(order.value()[0] == 0);
    REQUIRE(order.value()[1] == 1);
    REQUIRE(order.value()[2] == 2);
}

TEST_CASE("CompositeModule topologicalOrder handles diamond DAG with lexicographic tie-breaking", "[composite]") {
    gef::CompositeModule cm;
    
    // Diamond: 0 -> {1, 2} -> 3
    // Nodes 1 and 2 have no dependencies after 0 is processed
    // Must use lexicographic ordering on instance_name for tie-breaking
    gef::addChild(cm, "root", 10);
    gef::addChild(cm, "zebra", 20);   // id 1
    gef::addChild(cm, "alpha", 30);   // id 2
    gef::addChild(cm, "sink", 40);

    gef::addEdge(cm, 0, 1);  // root -> zebra
    gef::addEdge(cm, 0, 2);  // root -> alpha
    gef::addEdge(cm, 1, 3);  // zebra -> sink
    gef::addEdge(cm, 2, 3);  // alpha -> sink

    auto order = gef::topologicalOrder(cm);
    REQUIRE(order.has_value());
    REQUIRE(order.value().size() == 4);
    
    // Expected order: root (0), alpha (2), zebra (1), sink (3)
    // After root is processed, both alpha and zebra have in-degree 0
    // Lexicographic tie-breaking: "alpha" < "zebra"
    REQUIRE(order.value()[0] == 0);  // root
    REQUIRE(order.value()[1] == 2);  // alpha (lexicographically before zebra)
    REQUIRE(order.value()[2] == 1);  // zebra
    REQUIRE(order.value()[3] == 3);  // sink
}

TEST_CASE("CompositeModule topologicalOrder handles independent nodes with lexicographic ordering", "[composite]") {
    gef::CompositeModule cm;
    
    // All independent nodes - should be ordered lexicographically by instance_name
    gef::addChild(cm, "charlie", 10);
    gef::addChild(cm, "alice", 20);
    gef::addChild(cm, "bob", 30);

    auto order = gef::topologicalOrder(cm);
    REQUIRE(order.has_value());
    REQUIRE(order.value().size() == 3);
    
    // Expected order: alice (1), bob (2), charlie (0)
    REQUIRE(order.value()[0] == 1);  // alice
    REQUIRE(order.value()[1] == 2);  // bob
    REQUIRE(order.value()[2] == 0);  // charlie
}

TEST_CASE("CompositeModule topologicalOrder rejects local cycles", "[composite]") {
    gef::CompositeModule cm;
    
    gef::addChild(cm, "a", 10);
    gef::addChild(cm, "b", 20);
    gef::addChild(cm, "c", 30);

    // Create cycle: a -> b -> c -> a
    gef::addEdge(cm, 0, 1);
    gef::addEdge(cm, 1, 2);
    gef::addEdge(cm, 2, 0);

    auto order = gef::topologicalOrder(cm);
    REQUIRE(!order.has_value());
    REQUIRE(order.error().code == gef::ErrorCode::InvalidTopology);
    REQUIRE(order.error().message.find("Cycle") != std::string::npos);
}

TEST_CASE("CompositeModule topologicalOrder handles self-loop rejection via addEdge", "[composite]") {
    gef::CompositeModule cm;
    
    gef::addChild(cm, "node", 10);

    // Attempt to create self-loop (should be rejected by addEdge)
    auto self_edge = gef::addEdge(cm, 0, 0);
    REQUIRE(!self_edge.has_value());
    REQUIRE(self_edge.error().code == gef::ErrorCode::InvalidTopology);
    
    // Topology should still be valid (empty graph)
    auto order = gef::topologicalOrder(cm);
    REQUIRE(order.has_value());
    REQUIRE(order.value().size() == 1);
    REQUIRE(order.value()[0] == 0);
}

TEST_CASE("CompositeModule validateCompositeTopology accepts valid raw graphs", "[composite]") {
    gef::CompositeModule empty{};
    auto empty_result = gef::validateCompositeTopology(empty);
    REQUIRE(empty_result.has_value());

    gef::CompositeModule single{.children = {{"inst", 10}}, .edges = {}};
    auto single_result = gef::validateCompositeTopology(single);
    REQUIRE(single_result.has_value());

    gef::CompositeModule linear{
        .children = {{"a", 10}, {"b", 20}, {"c", 30}},
        .edges = {{0, 1}, {1, 2}}
    };
    auto linear_result = gef::validateCompositeTopology(linear);
    REQUIRE(linear_result.has_value());

    gef::CompositeModule diamond{
        .children = {{"root", 10}, {"left", 20}, {"right", 30}, {"sink", 40}},
        .edges = {{0, 1}, {0, 2}, {1, 3}, {2, 3}}
    };
    auto diamond_result = gef::validateCompositeTopology(diamond);
    REQUIRE(diamond_result.has_value());
}

TEST_CASE("CompositeModule validateCompositeTopology rejects duplicate names and invalid edges", "[composite]") {
    gef::CompositeModule dup_names{
        .children = {{"inst", 10}, {"inst", 20}},
        .edges = {}
    };
    auto dup_result = gef::validateCompositeTopology(dup_names);
    REQUIRE(!dup_result.has_value());
    REQUIRE(dup_result.error().code == gef::ErrorCode::InvalidTopology);
    REQUIRE(dup_result.error().message.find("Duplicate instance name") != std::string::npos);

    gef::CompositeModule out_of_range_from{
        .children = {{"a", 10}, {"b", 20}},
        .edges = {{5, 1}}
    };
    auto out_from = gef::validateCompositeTopology(out_of_range_from);
    REQUIRE(!out_from.has_value());
    REQUIRE(out_from.error().code == gef::ErrorCode::InvalidTopology);
    REQUIRE(out_from.error().message.find("out of range") != std::string::npos);

    gef::CompositeModule out_of_range_to{
        .children = {{"a", 10}, {"b", 20}},
        .edges = {{0, 99}}
    };
    auto out_to = gef::validateCompositeTopology(out_of_range_to);
    REQUIRE(!out_to.has_value());
    REQUIRE(out_to.error().code == gef::ErrorCode::InvalidTopology);
    REQUIRE(out_to.error().message.find("out of range") != std::string::npos);

    gef::CompositeModule self_edge{
        .children = {{"a", 10}, {"b", 20}},
        .edges = {{1, 1}}
    };
    auto self_result = gef::validateCompositeTopology(self_edge);
    REQUIRE(!self_result.has_value());
    REQUIRE(self_result.error().code == gef::ErrorCode::InvalidTopology);
    REQUIRE(self_result.error().message.find("Self-edge") != std::string::npos);

    gef::CompositeModule dup_edge{
        .children = {{"a", 10}, {"b", 20}, {"c", 30}},
        .edges = {{0, 1}, {0, 1}}
    };
    auto dup_edge_result = gef::validateCompositeTopology(dup_edge);
    REQUIRE(!dup_edge_result.has_value());
    REQUIRE(dup_edge_result.error().code == gef::ErrorCode::InvalidTopology);
    REQUIRE(dup_edge_result.error().message.find("Duplicate edge") != std::string::npos);
}

TEST_CASE("CompositeModule validateCompositeTopology rejects local cycles", "[composite]") {
    gef::CompositeModule cycle3{
        .children = {{"a", 10}, {"b", 20}, {"c", 30}},
        .edges = {{0, 1}, {1, 2}, {2, 0}}
    };
    auto cycle3_result = gef::validateCompositeTopology(cycle3);
    REQUIRE(!cycle3_result.has_value());
    REQUIRE(cycle3_result.error().code == gef::ErrorCode::InvalidTopology);
    REQUIRE(cycle3_result.error().message.find("Cycle") != std::string::npos);

    gef::CompositeModule cycle2{
        .children = {{"x", 10}, {"y", 20}},
        .edges = {{0, 1}, {1, 0}}
    };
    auto cycle2_result = gef::validateCompositeTopology(cycle2);
    REQUIRE(!cycle2_result.has_value());
    REQUIRE(cycle2_result.error().code == gef::ErrorCode::InvalidTopology);
    REQUIRE(cycle2_result.error().message.find("Cycle") != std::string::npos);
}

static gef::ModuleDef makeAtomicModuleDef(std::string name, std::string version = "0.1.0") {
    return gef::ModuleDef{
        .name = std::move(name),
        .signature = gef::ModuleSignature{
            .version = std::move(version),
            .bindings = {},
        },
        .variant = gef::AtomicModule{},
    };
}

TEST_CASE("registerCompositeModule accepts valid composite with atomic child definitions", "[composite][registration]") {
    gef::ModuleStore store;

    auto atomic_id1 = gef::registerModule(store, makeAtomicModuleDef("atomic.module1"));
    auto atomic_id2 = gef::registerModule(store, makeAtomicModuleDef("atomic.module2"));

    gef::CompositeModule cm;
    auto child1 = gef::addChild(cm, "inst1", atomic_id1);
    auto child2 = gef::addChild(cm, "inst2", atomic_id2);
    REQUIRE(child1.has_value());
    REQUIRE(child2.has_value());

    auto edge_result = gef::addEdge(cm, *child1, *child2);
    REQUIRE(edge_result.has_value());

    gef::ModuleSignature sig{
        .version = "1.0.0",
        .bindings = {},
    };

    auto composite_id = gef::registerCompositeModule(
        store, "composite.pipeline", std::move(sig), std::move(cm));

    REQUIRE(composite_id.has_value());

    auto def = gef::getModule(store, *composite_id);
    REQUIRE(def.has_value());
    REQUIRE((*def)->name == "composite.pipeline");
    REQUIRE(std::holds_alternative<gef::CompositeModule>((*def)->variant));
}

TEST_CASE("registerCompositeModule accepts repeated child module_id under distinct names", "[composite][registration]") {
    gef::ModuleStore store;

    auto atomic_id = gef::registerModule(store, makeAtomicModuleDef("shared.module"));

    gef::CompositeModule cm;
    auto alias1 = gef::addChild(cm, "first_use", atomic_id);
    auto alias2 = gef::addChild(cm, "second_use", atomic_id);
    auto alias3 = gef::addChild(cm, "third_use", atomic_id);

    REQUIRE(alias1.has_value());
    REQUIRE(alias2.has_value());
    REQUIRE(alias3.has_value());

    gef::ModuleSignature sig{
        .version = "1.0.0",
        .bindings = {},
    };

    auto composite_id = gef::registerCompositeModule(
        store, "composite.multi_instance", std::move(sig), std::move(cm));

    REQUIRE(composite_id.has_value());

    auto def = gef::getModule(store, *composite_id);
    REQUIRE(def.has_value());

    auto* composite_ptr = std::get_if<gef::CompositeModule>(&(*def)->variant);
    REQUIRE(composite_ptr != nullptr);
    REQUIRE(composite_ptr->children.size() == 3);
    REQUIRE(composite_ptr->children[0].module_id == atomic_id);
    REQUIRE(composite_ptr->children[1].module_id == atomic_id);
    REQUIRE(composite_ptr->children[2].module_id == atomic_id);
}

TEST_CASE("registerCompositeModule rejects missing child module ids", "[composite][registration][error]") {
    gef::ModuleStore store;

    gef::ModuleId missing_id = 9999;

    gef::CompositeModule cm;
    auto child_result = gef::addChild(cm, "orphan", missing_id);
    REQUIRE(child_result.has_value());

    gef::ModuleSignature sig{
        .version = "1.0.0",
        .bindings = {},
    };

    auto composite_id = gef::registerCompositeModule(
        store, "composite.broken", std::move(sig), std::move(cm));

    REQUIRE(!composite_id.has_value());
    REQUIRE(composite_id.error().code == gef::ErrorCode::ModuleNotFound);
    REQUIRE(composite_id.error().message.find("9999") != std::string::npos);
    REQUIRE(composite_id.error().message.find("orphan") != std::string::npos);
}

TEST_CASE("registerCompositeModule rejects invalid topology before checking child references", "[composite][registration][error]") {
    gef::ModuleStore store;

    auto atomic_id1 = gef::registerModule(store, makeAtomicModuleDef("atomic.a"));
    auto atomic_id2 = gef::registerModule(store, makeAtomicModuleDef("atomic.b"));

    gef::CompositeModule cm;
    auto child1 = gef::addChild(cm, "dup_name", atomic_id1);
    auto child2 = gef::addChild(cm, "dup_name", atomic_id2);
    REQUIRE(child1.has_value());
    REQUIRE(child2.has_value());

    gef::ModuleSignature sig{
        .version = "1.0.0",
        .bindings = {},
    };

    auto composite_id = gef::registerCompositeModule(
        store, "composite.invalid", std::move(sig), std::move(cm));

    REQUIRE(!composite_id.has_value());
    REQUIRE(composite_id.error().code == gef::ErrorCode::InvalidTopology);
    REQUIRE(composite_id.error().message.find("Duplicate instance name") != std::string::npos);
}

TEST_CASE("registerCompositeModule accepts nested composite children", "[composite][registration]") {
    gef::ModuleStore store;

    auto atomic_id = gef::registerModule(store, makeAtomicModuleDef("atomic.leaf"));

    gef::CompositeModule inner_cm;
    auto inner_child = gef::addChild(inner_cm, "inner_inst", atomic_id);
    REQUIRE(inner_child.has_value());

    gef::ModuleSignature inner_sig{
        .version = "1.0.0",
        .bindings = {},
    };

    auto inner_composite_id = gef::registerCompositeModule(
        store, "composite.inner", std::move(inner_sig), std::move(inner_cm));
    REQUIRE(inner_composite_id.has_value());

    gef::CompositeModule outer_cm;
    auto outer_child = gef::addChild(outer_cm, "outer_inst", *inner_composite_id);
    REQUIRE(outer_child.has_value());

    gef::ModuleSignature outer_sig{
        .version = "1.0.0",
        .bindings = {},
    };

    auto outer_composite_id = gef::registerCompositeModule(
        store, "composite.outer", std::move(outer_sig), std::move(outer_cm));
    REQUIRE(outer_composite_id.has_value());

    auto def = gef::getModule(store, *outer_composite_id);
    REQUIRE(def.has_value());
    REQUIRE((*def)->name == "composite.outer");

    auto* composite_ptr = std::get_if<gef::CompositeModule>(&(*def)->variant);
    REQUIRE(composite_ptr != nullptr);
    REQUIRE(composite_ptr->children.size() == 1);
    REQUIRE(composite_ptr->children[0].module_id == *inner_composite_id);
}

TEST_CASE("registerCompositeModule rejects invalid referenced composites", "[composite][registration][error]") {
    gef::ModuleStore store;

    auto atomic_id = gef::registerModule(store, makeAtomicModuleDef("atomic.leaf"));

    gef::CompositeModule malformed_inner{
        .children = {{"dup", atomic_id}, {"dup", atomic_id}},
        .edges = {}
    };

    auto malformed_inner_id = gef::registerModule(store, gef::ModuleDef{
        .name = "composite.malformed.inner",
        .signature = gef::ModuleSignature{"1.0.0", {}},
        .variant = std::move(malformed_inner),
    });

    gef::CompositeModule outer;
    auto outer_child = gef::addChild(outer, "outer_ref", malformed_inner_id);
    REQUIRE(outer_child.has_value());

    auto outer_id = gef::registerCompositeModule(
        store, "composite.outer.invalid_ref", gef::ModuleSignature{"1.0.0", {}}, std::move(outer));

    REQUIRE(!outer_id.has_value());
    REQUIRE(outer_id.error().code == gef::ErrorCode::InvalidTopology);
    REQUIRE(outer_id.error().message.find("invalid topology") != std::string::npos);
}

TEST_CASE("registerCompositeModule defensively rejects composite-reference cycles seeded via raw registerModule", "[composite][registration][error]") {
    gef::ModuleStore store;

    auto seed_atomic_id = gef::registerModule(store, makeAtomicModuleDef("atomic.seed"));
    REQUIRE(seed_atomic_id == 0);

    auto cycle_a_id = static_cast<gef::ModuleId>(store.defs.size());
    auto cycle_b_id = static_cast<gef::ModuleId>(cycle_a_id + 1);

    gef::CompositeModule cycle_a{
        .children = {{"to_b", cycle_b_id}},
        .edges = {}
    };
    gef::CompositeModule cycle_b{
        .children = {{"to_a", cycle_a_id}},
        .edges = {}
    };

    auto registered_cycle_a_id = gef::registerModule(store, gef::ModuleDef{
        .name = "composite.cycle.a",
        .signature = gef::ModuleSignature{"1.0.0", {}},
        .variant = std::move(cycle_a),
    });
    auto registered_cycle_b_id = gef::registerModule(store, gef::ModuleDef{
        .name = "composite.cycle.b",
        .signature = gef::ModuleSignature{"1.0.0", {}},
        .variant = std::move(cycle_b),
    });

    REQUIRE(registered_cycle_a_id == cycle_a_id);
    REQUIRE(registered_cycle_b_id == cycle_b_id);

    gef::CompositeModule outer;
    auto outer_child = gef::addChild(outer, "cycle_entry", registered_cycle_a_id);
    REQUIRE(outer_child.has_value());

    auto outer_id = gef::registerCompositeModule(
        store, "composite.outer.cycle_ref", gef::ModuleSignature{"1.0.0", {}}, std::move(outer));

    REQUIRE(!outer_id.has_value());
    REQUIRE(outer_id.error().code == gef::ErrorCode::InvalidTopology);
    REQUIRE(outer_id.error().message.find("Cycle") != std::string::npos);
}

TEST_CASE("registerCompositeModule preserves existing registerModule error semantics", "[composite][registration][error]") {
    gef::ModuleStore store;

    auto atomic_id = gef::registerModule(store, makeAtomicModuleDef("atomic.base"));

    gef::CompositeModule cm;
    auto child = gef::addChild(cm, "inst", atomic_id);
    REQUIRE(child.has_value());

    gef::ModuleSignature sig{
        .version = "1.0.0",
        .bindings = {},
    };

    REQUIRE_THROWS_AS(
        gef::registerCompositeModule(store, "", std::move(sig), std::move(cm)),
        std::invalid_argument
    );

    auto first_id = gef::registerCompositeModule(
        store, "composite.unique", gef::ModuleSignature{"1.0.0", {}},
        gef::CompositeModule{});
    REQUIRE(first_id.has_value());

    REQUIRE_THROWS_AS(
        gef::registerCompositeModule(
            store, "composite.unique", gef::ModuleSignature{"2.0.0", {}},
            gef::CompositeModule{}),
        std::invalid_argument
    );
}
