#ifndef GEF_FLOW_H_
#define GEF_FLOW_H_

#include <gef/core/binding/Context.h>
#include <gef/core/module/ModuleRegistry.h>
#include <gef/core/module/ModuleVariant.h>
#include <gef/core/system/Error.h>
#include <any>
#include <expected>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace gef {

struct FlowDataEdge {
    std::string from;
    std::string to;
    std::function<void(std::unordered_map<std::string, std::any>&)> allocate;
    std::function<void(std::unordered_map<std::string, std::any>&, Context&,
                       const std::string&)>
        bind;
    std::function<void(std::unordered_map<std::string, std::any>&, Context&)> copy_to_external;
};

struct Flow {
    const ModuleRegistry* registry = nullptr;
    std::unordered_map<std::string, std::string> instances;
    std::unordered_map<std::string, std::any> config;
    std::vector<std::function<void(Context&)>> config_binders;
    std::vector<FlowDataEdge> edges;
    std::unordered_map<std::string, std::any> data_store;
};

[[nodiscard]] auto createFlow(const ModuleRegistry& registry) noexcept -> Flow;

[[nodiscard]] auto flowAddModule(Flow& flow, const std::string& instance_name,
                                 const std::string& module_name)
    -> std::expected<void, Error>;

[[nodiscard]] auto flowExecute(Flow& flow, Context& ctx) -> std::expected<void, Error>;

template <typename T>
[[nodiscard]] auto flowConnect(Flow& flow, const std::string& from,
                               const std::string& to)
    -> std::expected<void, Error>;

template <typename T>
void flowSetConfig(Flow& flow, const std::string& key, T value);

[[nodiscard]] auto flowValidateEndpoint(const Flow& flow,
                                        const std::string& endpoint,
                                        const char* label)
    -> std::expected<void, Error>;

[[nodiscard]] auto flowParseEndpoint(const std::string& endpoint)
    -> std::expected<std::pair<std::string, std::string>, Error>;

template <typename T>
auto flowConnect(Flow& flow, const std::string& from, const std::string& to)
    -> std::expected<void, Error> {
    if (auto result = flowValidateEndpoint(flow, from, "from"); !result) {
        return std::unexpected(std::move(result.error()));
    }
    if (auto result = flowValidateEndpoint(flow, to, "to"); !result) {
        return std::unexpected(std::move(result.error()));
    }

    FlowDataEdge edge;
    edge.from = from;
    edge.to   = to;

    edge.allocate = [to](std::unordered_map<std::string, std::any>& store) {
        store[to] = T{};
    };

    edge.bind = [to](std::unordered_map<std::string, std::any>& store, Context& ctx,
                     const std::string& binding_name) {
        setBinding(ctx, binding_name,
                        std::any(&std::any_cast<T&>(store[to])));
    };

    edge.copy_to_external = [from, to](std::unordered_map<std::string, std::any>& store, Context& ctx) {
        auto dot_pos = to.find('.');
        auto to_instance = to.substr(0, dot_pos);
        auto to_binding = to.substr(dot_pos + 1);

        if (to_instance == "outputs") {
            T* ptr = std::any_cast<T*>(getBinding(ctx, to_binding));
            *ptr = std::any_cast<T>(store[to]);
        }
    };

    flow.edges.push_back(std::move(edge));
    return {};
}

template <typename T>
void flowSetConfig(Flow& flow, const std::string& key, T value) {
    flow.config[key] = std::move(value);
    flow.config_binders.push_back([key, &flow](Context& ctx) {
        setBinding(ctx, key, std::any(&std::any_cast<T&>(flow.config[key])));
    });
}

} // namespace gef

#endif
