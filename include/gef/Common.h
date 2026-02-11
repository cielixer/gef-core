#ifndef GEF_COMMON_H_
#define GEF_COMMON_H_

#include <cstdint>
#include <cstring>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Binding role enumeration
 *
 * Defines the kinds of bindings a module can have:
 * - INPUT: Read-only data from context
 * - OUTPUT: New data written to context
 * - INOUT: In-place mutation of existing data
 * - CONFIG: Immutable configuration settings
 */
typedef enum {
    GEF_ROLE_INPUT = 0,
    GEF_ROLE_OUTPUT = 1,
    GEF_ROLE_INOUT = 2,
    GEF_ROLE_CONFIG = 3
} gef_role_t;

/**
 * @brief Binding descriptor
 *
 * Describes a single input/output/inout/config binding of a module.
 */
typedef struct {
    const char* name;          /**< Binding name */
    gef_role_t role;          /**< Binding role (input/output/inout/config) */
    const char* type_name;    /**< Type name (e.g., "float", "std::vector<int>") */
} gef_binding_t;

/**
 * @brief Module metadata structure
 *
 * Encapsulates all static metadata about an atomic module,
 * including name, version, and binding declarations.
 */
typedef struct {
    const char* module_name;   /**< Module name */
    const char* version;       /**< Module version */
    const gef_binding_t* bindings;  /**< Array of binding descriptors */
    size_t num_bindings;       /**< Number of bindings */
} gef_metadata_t;

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // GEF_COMMON_H_
