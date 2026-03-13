#ifndef GEF_MODULE_H_
#define GEF_MODULE_H_

#include <gef/core/binding/Common.h>
#include <gef/core/binding/Context.h>

typedef void (*gef_execute_fn)(gef::Context& ctx);

#ifdef __cplusplus
extern "C" {
#endif

extern const gef_metadata_t* gef_get_metadata(void);
extern void gef_execute(gef::Context& ctx);

#ifdef __cplusplus
}
#endif

#endif
