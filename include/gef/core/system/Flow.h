#ifndef GEF_FLOW_H_
#define GEF_FLOW_H_

#include <gef/core/binding/Context.h>
#include <gef/core/module/ModuleStore.h>
#include <gef/core/module/Module.h>
#include <any>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace gef {

class Flow {
  public:
    explicit Flow(const ModuleStore& store) noexcept;

    void addModule(const std::string& instanceName, const std::string& moduleName);
    void execute(Context& ctx);

    template <typename T>
    void connect(const std::string& from, const std::string& to) {
        validateEndpoint(from, "from");
        validateEndpoint(to, "to");

        Edge edge;
        edge.from = from;
        edge.to   = to;

        edge.allocate = [to](std::map<std::string, std::any>& store) {
            store[to] = T{};
        };

        edge.bind = [to](std::map<std::string, std::any>& store, Context& ctx,
                         const std::string& binding_name) {
            ctx.set_binding(binding_name,
                            std::any(&std::any_cast<T&>(store[to])));
        };
        
        edge.copy_to_external = [from, to](std::map<std::string, std::any>& store, Context& ctx) {
            size_t dot_pos = to.find('.');
            std::string to_instance = to.substr(0, dot_pos);
            std::string to_binding = to.substr(dot_pos + 1);
            
            if (to_instance == "outputs") {
                T* ptr = std::any_cast<T*>(ctx.get_binding(to_binding));
                *ptr = std::any_cast<T>(store[to]);
            }
        };

        this->edges_.push_back(std::move(edge));
    }

    template <typename T>
    void setConfig(const std::string& key, T value) {
        this->config_[key] = std::move(value);
        this->config_binders_.push_back([key, this](Context& ctx) {
            ctx.set_binding(key, std::any(&std::any_cast<T&>(this->config_[key])));
        });
    }

  private:
    struct Edge {
        std::string from;
        std::string to;
        std::function<void(std::map<std::string, std::any>&)> allocate;
        std::function<void(std::map<std::string, std::any>&, Context&,
                           const std::string&)>
            bind;
        std::function<void(std::map<std::string, std::any>&, Context&)> copy_to_external;
    };

    void validateEndpoint(const std::string& endpoint, const char* label);

    [[nodiscard]] std::pair<std::string, std::string> parseEndpoint(const std::string& endpoint);

    const ModuleStore& store_;
    std::map<std::string, std::string> instances_;
    std::map<std::string, std::any> config_;
    std::vector<std::function<void(Context&)>> config_binders_;
    std::vector<Edge> edges_;
    std::map<std::string, std::any> data_store_;
};

}

#endif
