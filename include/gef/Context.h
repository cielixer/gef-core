#ifndef GEF_CONTEXT_H_
#define GEF_CONTEXT_H_

#include <any>
#include <map>
#include <string>

namespace gef {

template <typename T> class InputRef {
  public:
    InputRef(std::any* data) : data_(data) {}

    T* operator->() const {
        return std::any_cast<T*>(*data_);
    }

    T& operator*() const {
        return *std::any_cast<T*>(*data_);
    }

  private:
    std::any* data_;
};

template <typename T> class OutputRef {
  public:
    OutputRef(std::any* data) : data_(data) {}

    T* operator->() const {
        return std::any_cast<T*>(*data_);
    }

    T& operator*() const {
        return *std::any_cast<T*>(*data_);
    }

  private:
    std::any* data_;
};

template <typename T> class InOutRef {
  public:
    InOutRef(std::any* data) : data_(data) {}

    T* operator->() const {
        return std::any_cast<T*>(*data_);
    }

    T& operator*() const {
        return *std::any_cast<T*>(*data_);
    }

  private:
    std::any* data_;
};

template <typename T> class ConfigRef {
  public:
    ConfigRef(std::any* data) : data_(data) {}

    T* operator->() const {
        return std::any_cast<T*>(*data_);
    }

    T& operator*() const {
        return *std::any_cast<T*>(*data_);
    }

  private:
    std::any* data_;
};

class Context {
  public:
    Context()          = default;
    virtual ~Context() = default;

    template <typename T> InputRef<T> input(const char* name) {
        return InputRef<T>(&bindings_[name]);
    }

    template <typename T> OutputRef<T> output(const char* name) {
        return OutputRef<T>(&bindings_[name]);
    }

    template <typename T> InOutRef<T> inout(const char* name) {
        return InOutRef<T>(&bindings_[name]);
    }

    template <typename T> ConfigRef<T> config(const char* name) {
        return ConfigRef<T>(&bindings_[name]);
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
