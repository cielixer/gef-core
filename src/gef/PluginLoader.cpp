#include <cstring>
#include <gef/PluginLoader.h>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#define dlopen(path, flags) LoadLibraryA(path)
#define dlsym(handle, symbol) GetProcAddress((HMODULE)handle, symbol)
#define dlclose(handle) FreeLibrary((HMODULE)handle)
#define RTLD_LAZY 0
#else
#include <dlfcn.h>
#endif

namespace gef {

PluginLoader::PluginLoader() = default;

PluginLoader::~PluginLoader() = default;

void* PluginLoader::load(const char* path) {
    if (!path) {
        throw std::invalid_argument("Plugin path cannot be null");
    }

    void* handle = dlopen(path, RTLD_LAZY);
    if (!handle) {
#ifdef _WIN32
        throw std::runtime_error("Failed to load plugin: " + std::string(path));
#else
        throw std::runtime_error(std::string("Failed to load plugin: ") + dlerror());
#endif
    }

    return handle;
}

void* PluginLoader::getSymbol(void* handle, const char* symbol) {
    if (!handle) {
        throw std::invalid_argument("Plugin handle cannot be null");
    }
    if (!symbol) {
        throw std::invalid_argument("Symbol name cannot be null");
    }

    void* sym = dlsym(handle, symbol);
    if (!sym) {
#ifdef _WIN32
        throw std::runtime_error(std::string("Failed to find symbol: ") + symbol);
#else
        throw std::runtime_error(std::string("Failed to find symbol: ") + dlerror());
#endif
    }

    return sym;
}

void PluginLoader::unload(void* handle) {
    if (!handle) {
        return; // Silently ignore null handles
    }

    if (dlclose(handle) != 0) {
#ifdef _WIN32
        // Windows FreeLibrary doesn't provide detailed error messages
#else
        // Log but don't throw on unload failure
#endif
    }
}

} // namespace gef
