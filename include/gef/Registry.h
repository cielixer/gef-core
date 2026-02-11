#ifndef GEF_REGISTRY_H_
#define GEF_REGISTRY_H_

#include <gef/Common.h>
#include <map>
#include <string>

namespace gef {

class Registry {
public:
    Registry();
    ~Registry();
    
    void registerModule(const gef_metadata_t* metadata);
    const gef_metadata_t* getModule(const char* name);
    
private:
    std::map<std::string, const gef_metadata_t*> modules_;
};

}

#endif
