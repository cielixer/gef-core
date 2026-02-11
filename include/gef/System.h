#ifndef GEF_SYSTEM_H_
#define GEF_SYSTEM_H_

#include <gef/Context.h>
#include <gef/PluginLoader.h>
#include <gef/Registry.h>
#include <memory>

namespace gef {

class System {
  public:
    System();
    ~System();

    void loadModule(const char* path);
    void executeModule(const char* name, Context& ctx);

  private:
    std::unique_ptr<PluginLoader> loader_;
    std::unique_ptr<Registry> registry_;
};

} // namespace gef

#endif
