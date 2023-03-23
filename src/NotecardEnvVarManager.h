#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct NotecardEnvVarManager;
typedef struct NotecardEnvVarManager NotecardEnvVarManager;

typedef int (*envFetchCb)(const char *var, const char *val, void *userCtx);

NotecardEnvVarManager *NotecardEnvVarManager_alloc(void);
int NotecardEnvVarManager_enableCache(NotecardEnvVarManager *man);
void NotecardEnvVarManager_free(NotecardEnvVarManager *man);
int NotecardEnvVarManager_get(NotecardEnvVarManager *man, const char *var,
                              char *valBuf, size_t valBufSz);
int NotecardEnvVarManager_process(NotecardEnvVarManager *man);
int NotecardEnvVarManager_setEnvFetchCb(NotecardEnvVarManager *man,
                                        envFetchCb cb, void *userCtx);
int NotecardEnvVarManager_setProcessInterval(NotecardEnvVarManager *man,
        uint32_t interval);
int NotecardEnvVarManager_setWatchVars(NotecardEnvVarManager *man,
                                       const char **watchVars, size_t numWatchVars);

// Make these normally static functions externally visible if building tests.
#ifdef NEVM_TEST
int _envModified(NotecardEnvVarManager *man);
#endif

#ifdef __cplusplus
}
#endif
