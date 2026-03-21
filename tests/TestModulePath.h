#ifndef GEF_TEST_MODULE_PATH_H_
#define GEF_TEST_MODULE_PATH_H_

#include <filesystem>
#include <string>

// Helper to construct module file paths using CMake-provided prefix/suffix values.
// This allows tests to remain platform-independent without hardcoding .so, .dylib, etc.
inline std::filesystem::path getModulePath(const std::string& name) {
    return std::filesystem::path(GEF_MODULE_DIR) / (std::string(GEF_MODULE_PREFIX) + name +
                                                     std::string(GEF_MODULE_SUFFIX));
}

#endif // GEF_TEST_MODULE_PATH_H_
