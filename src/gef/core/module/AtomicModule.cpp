#include <gef/core/module/AtomicModule.h>
#include <dlfcn.h>
#include <spdlog/spdlog.h>
#include <format>
#include <stdexcept>

namespace fs = std::filesystem;

namespace gef {

AtomicModule::~AtomicModule() noexcept {
    if (handle && dlclose(handle) != 0) {
        spdlog::warn("Failed to unload module: {}", dlerror());
    }
}

auto loadAtomicModule(const fs::path& path)
    -> std::expected<AtomicModule, Error> {
    if (path.empty()) [[unlikely]] {
        throw std::invalid_argument("Module path cannot be empty");
    }

    if (!fs::exists(path)) [[unlikely]] {
        return std::unexpected(Error{ErrorCode::FileNotFound,
                                     "Module file does not exist: " + path.string()});
    }

    if (!fs::is_regular_file(path)) [[unlikely]] {
        return std::unexpected(Error{ErrorCode::InvalidFileType,
                                     "Module path is not a regular file: " + path.string()});
    }

    auto handle = dlopen(path.c_str(), RTLD_NOW);
    if (!handle) [[unlikely]] {
        return std::unexpected(Error{ErrorCode::LoadFailed,
                                     std::format("Failed to load module: {}", dlerror())});
    }

    using GetMetadataFunc = const gef_metadata_t* (*)();

    auto get_metadata_sym = dlsym(handle, "gef_get_metadata");
    if (!get_metadata_sym) [[unlikely]] {
        dlclose(handle);
        return std::unexpected(Error{ErrorCode::SymbolNotFound,
                                     std::format("Failed to find symbol: {}", dlerror())});
    }

    auto get_metadata = reinterpret_cast<GetMetadataFunc>(get_metadata_sym);
    auto metadata = get_metadata();
    if (!metadata) [[unlikely]] {
        dlclose(handle);
        return std::unexpected(Error{ErrorCode::MetadataInvalid,
                                     "Failed to retrieve module metadata"});
    }

    auto execute_sym = dlsym(handle, "gef_execute");
    if (!execute_sym) [[unlikely]] {
        dlclose(handle);
        return std::unexpected(Error{ErrorCode::SymbolNotFound,
                                     std::format("Failed to find symbol: {}", dlerror())});
    }

    auto execute = reinterpret_cast<gef_execute_fn_t>(execute_sym);

    spdlog::info("Loaded atomic module '{}' from {}", metadata->module_name, path.string());
    return AtomicModule{handle, metadata, execute};
}

}
