#ifndef GEF_PLUGINLOADER_H_
#define GEF_PLUGINLOADER_H_

namespace gef {

class PluginLoader {
public:
    PluginLoader();
    ~PluginLoader();
    
    void* load(const char* path);
    void* getSymbol(void* handle, const char* symbol);
    void unload(void* handle);
};

}

#endif
