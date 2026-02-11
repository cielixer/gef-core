#ifndef GEF_REGISTRY_H_
#define GEF_REGISTRY_H_

#include <gef/Common.h>
#include <gef/Module.h>
#include <map>
#include <string>

namespace gef {

struct ModuleEntry {
    const gef_metadata_t* metadata;
    gef_execute_fn execute;
};

class Registry {
  public:
    Registry();
    ~Registry();

    void registerModule(const gef_metadata_t* metadata, gef_execute_fn execute);
    const ModuleEntry* getModule(const char* name);

  private:
    std::map<std::string, ModuleEntry> modules_;
};

} // namespace gef

#endif
