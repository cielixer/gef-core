#ifndef GEF_SYSTEM_H_
#define GEF_SYSTEM_H_

#include <gef/PluginLoader.h>
#include <gef/Registry.h>
#include <gef/Context.h>
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

}

#endif
