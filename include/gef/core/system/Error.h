#ifndef GEF_ERROR_H_
#define GEF_ERROR_H_

#include <string>

namespace gef {

/// Error codes for expected (recoverable) failure paths.
enum class ErrorCode {
    FileNotFound,
    InvalidFileType,
    LoadFailed,
    SymbolNotFound,
    MetadataInvalid,
    ModuleNotFound,
    InvalidTopology,
};

/// Lightweight, copyable error type for use with std::expected.
struct Error {
    ErrorCode code;
    std::string message;
};

} // namespace gef

#endif
