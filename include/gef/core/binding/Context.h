#ifndef GEF_CONTEXT_H_
#define GEF_CONTEXT_H_

#include <any>
#include <string>
#include <unordered_map>

namespace gef {

struct Context {
    std::unordered_map<std::string, std::any> bindings;
};

template <typename T>
[[nodiscard]] auto ctxInput(Context& ctx, const char* name) -> const T& {
    return *std::any_cast<T*>(ctx.bindings[name]);
}

template <typename T>
[[nodiscard]] auto ctxOutput(Context& ctx, const char* name) -> T& {
    return *std::any_cast<T*>(ctx.bindings[name]);
}

template <typename T>
[[nodiscard]] auto ctxInout(Context& ctx, const char* name) -> T& {
    return *std::any_cast<T*>(ctx.bindings[name]);
}

template <typename T>
[[nodiscard]] auto ctxConfig(Context& ctx, const char* name) -> const T& {
    return *std::any_cast<T*>(ctx.bindings[name]);
}

[[nodiscard]] inline auto getBinding(Context& ctx, const std::string& name) -> std::any& {
    return ctx.bindings[name];
}

inline void setBinding(Context& ctx, const std::string& name, std::any value) {
    ctx.bindings[name] = std::move(value);
}

} // namespace gef

#endif
