#ifndef GEF_ERROR_H_
#define GEF_ERROR_H_

#include <string>

namespace gef {

enum class ErrorCode {
    FileNotFound,
    InvalidFileType,
    LoadFailed,
    SymbolNotFound,
    MetadataInvalid,
    ModuleNotFound,
    CycleDetected,
    DuplicateInstance,
    InvalidEndpoint,
    ExecutionFailed,
};

struct Error {
    ErrorCode code;
    std::string message;
};

} // namespace gef

#endif
