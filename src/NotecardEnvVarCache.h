#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct NotecardEnvVarCache;
typedef struct NotecardEnvVarCache NotecardEnvVarCache;

void NotecardEnvVarCache_free(NotecardEnvVarCache *cache);
NotecardEnvVarCache *NotecardEnvVarCache_new(void);
void NotecardEnvVarCache_free(NotecardEnvVarCache *cache);
const char *NotecardEnvVarCache_get(NotecardEnvVarCache *cache, const char *var);
bool NotecardEnvVarCache_update(NotecardEnvVarCache *cache, const char *var,
                                const char *val);

#ifdef __cplusplus
}
#endif
