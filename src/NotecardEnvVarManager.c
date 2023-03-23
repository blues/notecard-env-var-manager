#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "note-c/note.h"

#include "log.h"
#include "NotecardEnvVarCache.h"
#include "NotecardEnvVarManager.h"

struct NotecardEnvVarManager {
    NotecardEnvVarCache *cache;
    const char **watchVars;
    envFetchCb fetchCb;
    void *userCtx;
    size_t numWatchVars;
    uint32_t processIntervalMs;
    uint32_t lastProcessCallMs;
    uint32_t envLastModTime;
};

/**
 * Internal function to check if the environment variables for the Notehub
 * project have been modified.
 *
 * @param man Pointer to a NotecardEnvVarManager object.
 *
 * @return NEVM_SUCCESS if the environment has been modified and NEVM_FAILURE
 *         otherwise.
 */
NEVM_STATIC int _envModified(NotecardEnvVarManager *man)
{
    int ret = NEVM_FAILURE;
    J *rsp = NoteRequestResponse(NoteNewRequest("env.modified"));

    if (rsp != NULL && JHasObjectItem(rsp, "time")) {
        uint32_t modifiedTime = JGetInt(rsp, "time");
        NoteDeleteResponse(rsp);

        if (man->envLastModTime != modifiedTime) {
            man->envLastModTime = modifiedTime;
            ret = NEVM_SUCCESS;
        }
    }

    return ret;
}

/**
 * Internal function to get the watched environment variables, or all
 * environment variables, if the array of watched variables is NULL. The
 * user-provided environment fetch callback will be called on each variable, if
 * the callback is not NULL.
 *
 * @param man Pointer to a NotecardEnvVarManager object.
 *
 * @return NEVM_SUCCESS on success and negative values on failure.
 */
NEVM_STATIC int _fetchEnv(NotecardEnvVarManager *man)
{
    int ret = NEVM_FAILURE;
    J *req = NoteNewRequest("env.get");

    if (req != NULL) {
        if (man->watchVars != NULL) {
            J *names = JAddArrayToObject(req, "names");

            // If watchVars have been specified, filter on those. If there are
            // no watchVars, all environment variables will be returned by
            // Notehub.
            for (size_t i = 0; i < man->numWatchVars; ++i) {
                JAddItemToArray(names, JCreateString(man->watchVars[i]));
            }
        }

        J *rsp = NoteRequestResponse(req);
        if (rsp != NULL) {
            if (NoteResponseError(rsp)) {
                NoteDeleteResponse(rsp);
            } else {
                ret = NEVM_SUCCESS;
                J *body = JGetObject(rsp, "body");
                if (body != NULL && man->fetchCb != NULL) {
                    J *item = NULL;
                    JObjectForEach(item, body) {
                        char *var = item->string;
                        char *val = JGetStringValue(item);
                        // Call the user's fetchCb on each variable:value pair
                        // in the response.
                        if (man->fetchCb(var, val, man->userCtx)
                                != NEVM_SUCCESS) {
                            ret = NEVM_FAILURE;
                            break;
                        }

                        if (man->cache != NULL) {
                            // TODO: Check return.
                            NotecardEnvVarCache_update(man->cache, var, val);
                        }
                    }
                }
            }
        }
    }

    return ret;
}

/**
 * Fetch the value of an environment variable from Notehub.
 *
 * @param man      Pointer to a NotecardEnvVarManager object.
 * @param var      The variable to get the value of.
 * @param valBuf   Buffer to copy the value of the variable into.
 * @param valBufSz The size of valBuf in bytes. If valBuf isn't big enough to
 *                 hold the value, this function will return an error.
 *
 * @return NEVM_SUCCESS on success and negative values on errors.
 */
NEVM_STATIC int _fetchVar(NotecardEnvVarManager* man, const char* var,
                          char *valBuf, size_t valBufSz)
{
    J *req = NoteNewRequest("env.get");
    if (req != NULL) {
        JAddStringToObject(req, "name", var);
        J *rsp = NoteRequestResponse(req);
        if (rsp != NULL) {
            if (!NoteResponseError(rsp)) {
                char *text = JGetString(rsp, "text");
                if (text != NULL) {
                    size_t textLen = strlen(text);
                    if (textLen < valBufSz) {
                        strlcpy(valBuf, text, valBufSz);
                        NoteDeleteResponse(rsp);

                        if (man->fetchCb != NULL && man->fetchCb(var, valBuf,
                                man->userCtx) != NEVM_SUCCESS) {
                            // TODO: Should this be a distinct error code?
                            return NEVM_FAILURE;
                        }
                    } else {
                        LOG_ERROR("Value buffer size %u is too small for value "
                                  "\"%s\".", valBufSz, text);
                        NoteDeleteResponse(rsp);
                        return NEVM_FAILURE;
                    }
                } else {
                    LOG_ERROR("No \"text\" field in env.get response.");
                    NoteDeleteResponse(rsp);
                    return NEVM_FAILURE;
                }
            } else {
                LOG_ERROR("Error in env.get response.");
                NoteDeleteResponse(rsp);
                return NEVM_FAILURE;
            }
        }
    } else {
        LOG_ERROR("Out of memory.");
        return NEVM_FAILURE;
    }

    return NEVM_SUCCESS;
}

/**
 * Internal function to check if a variable is watched by the manager.
 *
 * @param man Pointer to a NotecardEnvVarManager object.
 * @param var Variable to check.
 *
 * @return True if the variable is watched and false otherwise.
 */
NEVM_STATIC bool _watched(const NotecardEnvVarManager *man, const char *var)
{
    bool watched = true;

    if (man->watchVars != NULL) {
        watched = false;
        for (size_t i = 0; i < man->numWatchVars; ++i) {
            if (strcmp(man->watchVars[i], var) == 0) {
                watched = true;
                break;
            }
        }
    }

    return watched;
}

/**
 * Free a NotecardEnvVarManager's memory.
 *
 * @param man Pointer to a NotecardEnvVarManager object.
 */
void NotecardEnvVarManager_free(NotecardEnvVarManager *man)
{
    if (man != NULL) {
        NotecardEnvVarCache_free(man->cache);
        NoteFree(man);
    }
}

/**
 * Get the value of an environment variable. If the cache is in use, this
 * function will update the cache, if it detects that the environment has been
 * modified. It will then grab the value for the variable from the cache. If
 * the cache isn't in use, the value for the variable will be fetched directly
 * from Notehub.
 *
 * @param man      Pointer to a NotecardEnvVarManager object.
 * @param var      The variable to get the value of.
 * @param valBuf   Buffer to copy the value of the variable into.
 * @param valBufSz The size of valBuf in bytes. If valBuf isn't big enough to
 *                 hold the value, this function will return an error.
 *
 * @return NEVM_SUCCESS on success and negative values on errors or if the
 *         variable doesn't exist.
 */
int NotecardEnvVarManager_get(NotecardEnvVarManager *man, const char *var,
                              char *valBuf, size_t valBufSz)
{
    if (man == NULL || var == NULL || valBuf == NULL || valBufSz == 0) {
        LOG_ERROR("Invalid parameter.");
        return NEVM_FAILURE;
    }

    // If watchVars have been specified, ensure the queried variable is in
    // watchVars before proceeding.
    if (!_watched(man, var)) {
        LOG_ERROR("Variable %s not watched.", var);
        return NEVM_FAILURE;
    }

    if (man->cache == NULL) {
        return _fetchVar(man, var, valBuf, valBufSz);
    } else {
        // If the environment has changed, fetch from Notehub.
        if (_envModified(man) == NEVM_SUCCESS &&
                _fetchEnv(man) != NEVM_SUCCESS) {
            LOG_ERROR("Failed to fetch environment.");
            return NEVM_FAILURE;
        }

        const char *cacheVal = NotecardEnvVarCache_get(man->cache, var);
        if (cacheVal == NULL) {
            LOG_ERROR("Variable %s doesn't exist.", var);
            return NEVM_FAILURE;
        }

        size_t valLen = strlen(cacheVal);
        if (valLen < valBufSz) {
            strlcpy(valBuf, cacheVal, valBufSz);
        } else {
            LOG_ERROR("Value buffer size %u is too small for value \"%s\".",
                      valBufSz, cacheVal);
            return NEVM_FAILURE;
        }
    }

    return NEVM_SUCCESS;
}

/**
 * Create a new NotecardEnvVarManager.
 *
 * @return A valid pointer to a NotecardEnvVarManager on success and NULL on
 *         failure.
 */
NotecardEnvVarManager *NotecardEnvVarManager_alloc(void)
{
    NotecardEnvVarManager *man = (NotecardEnvVarManager *)NoteMalloc(
                                     sizeof(NotecardEnvVarManager));
    if (man == NULL) {
        return NULL;
    }
    memset(man, 0, sizeof(*man));

    LOG_INFO("Successfully created new NotecardEnvVarManager.");

    return man;
}

/**
 * Enable the variable cache. When possible, values for environment variables
 * will be pulled from this local, in-memory cache instead of reaching out to
 * Notehub.
 *
 * @param man Pointer to a NotecardEnvVarManager object.
 *
 * @return A valid pointer to a NotecardEnvVarManager on success and NULL on
 *         failure.
 */
int NotecardEnvVarManager_enableCache(NotecardEnvVarManager *man)
{
    int ret = NEVM_FAILURE;

    if (man != NULL) {
        if (man->cache != NULL) {
            // Cache already enabled.
            ret = NEVM_SUCCESS;
        } else {
            NotecardEnvVarCache *cache = NotecardEnvVarCache_new();
            if (cache != NULL) {
                man->cache = cache;
                ret = NEVM_SUCCESS;
            }
        }
    }

    return ret;
}

/**
 * Set the environment variable manager's environment change callback and
 * user context.
 *
 * @param man     Pointer to a NotecardEnvVarManager object.
 * @param cb      An environment fetch callback.
 * @param userCtx Pointer to a user context.
 *
 * @return NEVM_SUCCESS on success and negative values on failure.
 */
int NotecardEnvVarManager_setEnvFetchCb(NotecardEnvVarManager *man,
                                        envFetchCb cb, void *userCtx)
{
    int ret = NEVM_FAILURE;

    if (man != NULL) {
        man->fetchCb = cb;
        man->userCtx = userCtx;
        ret = NEVM_SUCCESS;
    }

    return ret;
}

/**
 * Set the environment variable manager's process interval.
 *
 * @param man      Pointer to a NotecardEnvVarManager object.
 * @param interval The process interval, in seconds.
 *
 * @return NEVM_SUCCESS on success and negative values on failure.
 */
int NotecardEnvVarManager_setProcessInterval(NotecardEnvVarManager *man,
        uint32_t interval)
{
    int ret = NEVM_FAILURE;

    if (man != NULL) {
        man->processIntervalMs = interval * 1000;
        ret = NEVM_SUCCESS;
    }

    return ret;
}

/**
 * Set the environment variable manager's watch variables.
 *
 * @param man          Pointer to a NotecardEnvVarManager object.
 * @param watchVars    Pointer to an array of C-strings with the environment
 *                     variables to watch. If NULL, all environment variables
 *                     will be watched. The array is not copied, and
 *                     ownership transfers to the manager.
 * @param numWatchVars The number of variables in watchVars.
 *
 * @return NEVM_SUCCESS on success and negative values on failure.
 */
int NotecardEnvVarManager_setWatchVars(NotecardEnvVarManager *man,
                                       const char **watchVars, size_t numWatchVars)
{
    int ret = NEVM_FAILURE;

    if (man != NULL) {
        if (watchVars == NULL || numWatchVars == 0) {
            man->watchVars = NULL;
            man->numWatchVars = 0;
        } else {
            man->watchVars = watchVars;
            man->numWatchVars = numWatchVars;
        }
        ret = NEVM_SUCCESS;
    }

    return ret;
}

/**
 * Check for environment changes, if the process interval has lapsed. If the
 * environment has changed, get the watched variables from Notehub via the
 * Notecard, and call the user-provided environment change callback on each
 * variable.
 *
 * @param man Pointer to a NotecardEnvVarManager object.
 *
 * @return NEVM_SUCCESS on success and negative values on failure.
 */
int NotecardEnvVarManager_process(NotecardEnvVarManager *man)
{
    int ret = NEVM_FAILURE;

    if (man != NULL) {
        uint32_t currentMs = NoteGetMs();
        if (currentMs - man->lastProcessCallMs > man->processIntervalMs) {
            man->lastProcessCallMs = currentMs;
            LOG_DEBUG("Process interval lapsed. Processing...");
            if (_envModified(man) == NEVM_SUCCESS) {
                ret = _fetchEnv(man);
            } else {
                ret = NEVM_SUCCESS;
            }
        } else {
            ret = NEVM_SUCCESS;
        }
    }

    return ret;
}
