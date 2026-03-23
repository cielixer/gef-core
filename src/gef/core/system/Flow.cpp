#include <gef/core/system/Flow.h>
#include <gef/core/module/AtomicModule.h>
#include <gef/core/scheduler/Scheduler.h>

namespace gef {

auto createFlow(const ModuleRegistry& registry) noexcept -> Flow {
    return Flow{.registry = &registry, .instances = {}, .config = {}, .config_binders = {}, .edges = {}, .data_store = {}};
}

auto flowAddModule(Flow& flow, const std::string& instance_name,
                   const std::string& module_name)
    -> std::expected<void, Error> {
    if (flow.instances.contains(instance_name)) [[unlikely]] {
        return std::unexpected(Error{
            ErrorCode::DuplicateInstance,
            "Duplicate instance name: " + instance_name,
        });
    }

    auto id = registryFind(*flow.registry, module_name);
    if (!id) [[unlikely]] {
        return std::unexpected(Error{
            ErrorCode::ModuleNotFound,
            "Unknown module: " + module_name,
        });
    }

    flow.instances[instance_name] = module_name;
    return {};
}

auto flowExecute(Flow& flow, Context& ctx) -> std::expected<void, Error> {
    std::vector<std::pair<std::string, std::string>> dag_edges;
    std::vector<std::string> all_instances;
    all_instances.reserve(flow.instances.size());

    for (const auto& [instance_name, _] : flow.instances) {
        all_instances.push_back(instance_name);
    }

    for (const auto& edge : flow.edges) {
        auto from_ep = flowParseEndpoint(edge.from);
        if (!from_ep) [[unlikely]] {
            return std::unexpected(std::move(from_ep.error()));
        }
        auto to_ep = flowParseEndpoint(edge.to);
        if (!to_ep) [[unlikely]] {
            return std::unexpected(std::move(to_ep.error()));
        }

        auto& [from_instance, from_binding] = *from_ep;
        auto& [to_instance, to_binding] = *to_ep;

        if (from_instance != "inputs" && from_instance != "outputs" &&
            to_instance != "inputs" && to_instance != "outputs") {
            dag_edges.push_back({from_instance, to_instance});
        }
    }

    std::vector<std::string> execution_order;
    if (!all_instances.empty()) {
        auto sorted = topologicalSort(all_instances, dag_edges);
        if (!sorted) [[unlikely]] {
            return std::unexpected(std::move(sorted.error()));
        }
        execution_order = std::move(*sorted);
    }

    for (const auto& edge : flow.edges) {
        if (edge.allocate) {
            edge.allocate(flow.data_store);
        }
    }

    for (const std::string& instance_name : execution_order) {
        Context local_ctx;

        for (const auto& edge : flow.edges) {
            auto from_ep = flowParseEndpoint(edge.from);
            auto to_ep = flowParseEndpoint(edge.to);
            if (!from_ep || !to_ep) [[unlikely]] {
                continue;
            }

            auto& [from_instance, from_binding] = *from_ep;
            auto& [to_instance, to_binding] = *to_ep;

            if (to_instance == instance_name) {
                if (from_instance == "inputs") {
                    setBinding(local_ctx, to_binding, getBinding(ctx, from_binding));
                } else if (from_instance != "outputs") {
                    if (edge.bind) {
                        edge.bind(flow.data_store, local_ctx, to_binding);
                    }
                }
            }
        }

        for (const auto& edge : flow.edges) {
            auto from_ep = flowParseEndpoint(edge.from);
            auto to_ep = flowParseEndpoint(edge.to);
            if (!from_ep || !to_ep) [[unlikely]] {
                continue;
            }

            auto& [from_instance, from_binding] = *from_ep;
            auto& [to_instance, to_binding] = *to_ep;

            if (from_instance == instance_name) {
                if (to_instance != "inputs" && to_instance != "outputs") {
                    if (edge.bind) {
                        edge.bind(flow.data_store, local_ctx, from_binding);
                    }
                } else if (to_instance == "outputs") {
                    if (edge.bind) {
                        edge.bind(flow.data_store, local_ctx, from_binding);
                    }
                }
            }
        }

        for (const auto& config_binder : flow.config_binders) {
            config_binder(local_ctx);
        }

        auto it = flow.instances.find(instance_name);
        if (it == flow.instances.end()) [[unlikely]] {
            return std::unexpected(Error{
                ErrorCode::ModuleNotFound,
                "Instance not found: " + instance_name,
            });
        }

        auto id = registryFind(*flow.registry, it->second);
        if (!id) [[unlikely]] {
            return std::unexpected(Error{
                ErrorCode::ModuleNotFound,
                "Module not found: " + it->second,
            });
        }

        auto def = registryGet(*flow.registry, *id);
        if (!def) [[unlikely]] {
            return std::unexpected(Error{
                ErrorCode::ModuleNotFound,
                "Module def not found for id: " + std::to_string(*id),
            });
        }

        std::visit(overloaded{
            [&](const AtomicModule& a) { executeAtomicModule(a, local_ctx); },
            [&](const FlowModule&) {},
            [&](const PipelineModule&) {},
        }, (*def)->variant);
    }

    for (const auto& edge : flow.edges) {
        if (edge.copy_to_external) {
            edge.copy_to_external(flow.data_store, ctx);
        }
    }

    return {};
}

auto flowParseEndpoint(const std::string& endpoint)
    -> std::expected<std::pair<std::string, std::string>, Error> {
    auto dot_pos = endpoint.find('.');
    if (dot_pos == std::string::npos) [[unlikely]] {
        return std::unexpected(Error{
            ErrorCode::InvalidEndpoint,
            "Invalid endpoint format: " + endpoint,
        });
    }
    return std::pair{endpoint.substr(0, dot_pos), endpoint.substr(dot_pos + 1)};
}

auto flowValidateEndpoint(const Flow& flow, const std::string& endpoint,
                          const char* label)
    -> std::expected<void, Error> {
    auto dot_pos = endpoint.find('.');
    if (dot_pos == std::string::npos) [[unlikely]] {
        return std::unexpected(Error{
            ErrorCode::InvalidEndpoint,
            std::string(label) + " endpoint must contain '.': " + endpoint,
        });
    }

    auto instance = endpoint.substr(0, dot_pos);

    if (instance != "inputs" && instance != "outputs") {
        if (!flow.instances.contains(instance)) [[unlikely]] {
            return std::unexpected(Error{
                ErrorCode::InvalidEndpoint,
                "Unknown instance in " + std::string(label) +
                    " endpoint: " + instance,
            });
        }
    }

    return {};
}

} // namespace gef
