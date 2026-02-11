#ifndef GEF_CONTEXT_H_
#define GEF_CONTEXT_H_

#include <any>
#include <map>
#include <string>

namespace gef {

class Context {
  public:
    Context()          = default;
    virtual ~Context() = default;

    template <typename T> const T& input(const char* name) {
        return *std::any_cast<T*>(bindings_[name]);
    }

    template <typename T> T& output(const char* name) {
        return *std::any_cast<T*>(bindings_[name]);
    }

    template <typename T> T& inout(const char* name) {
        return *std::any_cast<T*>(bindings_[name]);
    }

    template <typename T> const T& config(const char* name) {
        return *std::any_cast<T*>(bindings_[name]);
    }

    std::any& get_binding(const std::string& name) {
        return bindings_[name];
    }

    void set_binding(const std::string& name, std::any value) {
        bindings_[name] = value;
    }

  private:
    std::map<std::string, std::any> bindings_;
};

} // namespace gef

#endif
