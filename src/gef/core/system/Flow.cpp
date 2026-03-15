#include <gef/core/system/Flow.h>
#include <gef/core/scheduler/Scheduler.h>
#include <stdexcept>
#include <set>

namespace gef {

Flow::Flow(const ModuleStore& store) noexcept
    : store_(store) {}

void Flow::addModule(const std::string& instanceName,
                     const std::string& moduleName) {
    if (this->instances_.contains(instanceName)) [[unlikely]] {
        throw std::runtime_error("Duplicate instance name: " + instanceName);
    }

    auto id = findModule(this->store_, moduleName);
    if (!id) [[unlikely]] {
        throw std::runtime_error("Unknown module: " + moduleName);
    }

    this->instances_[instanceName] = moduleName;
}

void Flow::execute(Context& ctx) {
    std::vector<std::pair<std::string, std::string>> dag_edges;
    std::set<std::string> all_instances;
    
    for (const auto& [instance_name, _] : this->instances_) {
        all_instances.insert(instance_name);
    }
    
    for (const auto& edge : this->edges_) {
        auto [from_instance, from_binding] = parseEndpoint(edge.from);
        auto [to_instance, to_binding] = parseEndpoint(edge.to);
        
        if (from_instance != "inputs" && from_instance != "outputs" &&
            to_instance != "inputs" && to_instance != "outputs") {
            dag_edges.push_back({from_instance, to_instance});
        }
    }
    
    std::vector<std::string> execution_order;
    if (!all_instances.empty()) {
        std::vector<std::string> instance_vec(all_instances.begin(), all_instances.end());
        execution_order = Scheduler::topologicalSort(instance_vec, dag_edges);
    }
    
    for (const auto& edge : this->edges_) {
        if (edge.allocate) {
            edge.allocate(this->data_store_);
        }
    }
    
    for (const std::string& instance_name : execution_order) {
        Context local_ctx;
        
        for (const auto& edge : this->edges_) {
            auto [from_instance, from_binding] = parseEndpoint(edge.from);
            auto [to_instance, to_binding] = parseEndpoint(edge.to);
            
            if (to_instance == instance_name) {
                if (from_instance == "inputs") {
                    local_ctx.set_binding(to_binding, ctx.get_binding(from_binding));
                } else if (from_instance != "outputs") {
                    if (edge.bind) {
                        edge.bind(this->data_store_, local_ctx, to_binding);
                    }
                }
            }
        }
        
        for (const auto& edge : this->edges_) {
            auto [from_instance, from_binding] = parseEndpoint(edge.from);
            auto [to_instance, to_binding] = parseEndpoint(edge.to);
            
            if (from_instance == instance_name) {
                if (to_instance != "inputs" && to_instance != "outputs") {
                    if (edge.bind) {
                        edge.bind(this->data_store_, local_ctx, from_binding);
                    }
                } else if (to_instance == "outputs") {
                    if (edge.bind) {
                        edge.bind(this->data_store_, local_ctx, from_binding);
                    }
                }
            }
        }
        
        for (const auto& config_binder : this->config_binders_) {
            config_binder(local_ctx);
        }
        
        auto id = findModule(this->store_, this->instances_[instance_name]);
        if (!id) [[unlikely]] {
            throw std::runtime_error("Module not found: " + this->instances_[instance_name]);
        }

        auto def = getModule(this->store_, *id);
        if (!def) [[unlikely]] {
            throw std::runtime_error("Module def not found for id: " + std::to_string(*id));
        }

        std::visit(overloaded{
            [&](const AtomicModule& a) { a.execute(local_ctx); },
            [&](const CompositeModule&) {},
        }, (*def)->variant);
    }
    
    for (const auto& edge : this->edges_) {
        if (edge.copy_to_external) {
            edge.copy_to_external(this->data_store_, ctx);
        }
    }
}

std::pair<std::string, std::string> Flow::parseEndpoint(const std::string& endpoint) {
    auto dot_pos = endpoint.find('.');
    if (dot_pos == std::string::npos) [[unlikely]] {
        throw std::runtime_error("Invalid endpoint format: " + endpoint);
    }
    return {endpoint.substr(0, dot_pos), endpoint.substr(dot_pos + 1)};
}

void Flow::validateEndpoint(const std::string& endpoint, const char* label) {
    auto dot_pos = endpoint.find('.');
    if (dot_pos == std::string::npos) [[unlikely]] {
        throw std::runtime_error(std::string(label) +
                                 " endpoint must contain '.': " + endpoint);
    }

    auto instance = endpoint.substr(0, dot_pos);

    if (instance != "inputs" && instance != "outputs") {
        if (!this->instances_.contains(instance)) [[unlikely]] {
            throw std::runtime_error("Unknown instance in " + std::string(label) +
                                     " endpoint: " + instance);
        }
    }
}

}
