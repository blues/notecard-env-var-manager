#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "note-c/note.h"

#include "defs.h"
#include "log.h"
#include "NotecardEnvVarCache.h"

NEVM_STATIC void _freeEntry(NotecardEnvVarCache *entry);

struct NotecardEnvVarCache {
    char *var;
    char *val;
    NotecardEnvVarCache *prev;
    NotecardEnvVarCache *next;
};

NEVM_STATIC bool _addEntry(NotecardEnvVarCache *cache, const char *var,
                           const char *val)
{
    bool success = true;

    NotecardEnvVarCache *entry;
    if (cache->var == NULL) {
        entry = cache;
    } else {
        entry = NotecardEnvVarCache_new();
    }

    if (entry != NULL) {
        entry->var = strdup(var);
        entry->val = strdup(val);
        if (entry->var == NULL || entry->val == NULL) {
            success = false;
        } else if (entry != cache) {
            if (cache->next != NULL) {
                cache->next->prev = entry;
            }
            entry->next = cache->next;
            entry->prev = cache;
            cache->next = entry;
        }
    } else {
        success = false;
    }

    if (!success) {
        _freeEntry(entry);
    }

    return success;
}

NEVM_STATIC void _freeEntry(NotecardEnvVarCache *entry)
{
    if (entry != NULL) {
        if (entry->prev != NULL) {
            entry->prev->next = entry->next;
        }
        if (entry->next != NULL) {
            entry->next->prev = entry->prev;
        }

        NoteFree(entry->var);
        NoteFree(entry->val);
        NoteFree(entry);
    }
}

void NotecardEnvVarCache_free(NotecardEnvVarCache *cache)
{
    NotecardEnvVarCache* next;
    while (cache != NULL) {
        next = cache->next;
        _freeEntry(cache);
        cache = next;
    }
}

const char *NotecardEnvVarCache_get(NotecardEnvVarCache *cache, const char *var)
{
    if (cache == NULL || var == NULL) {
        return NULL;
    }

    const char *val = NULL;
    while (cache != NULL && cache->var != NULL) {
        if (strcmp(var, cache->var) == 0) {
            val = cache->val;
            break;
        }
        cache = cache->next;
    }

    if (val == NULL) {
        LOG_ERROR("Variable %s not found in cache.", var);
    }

    return val;
}

NotecardEnvVarCache *NotecardEnvVarCache_new(void)
{
    NotecardEnvVarCache *cache = (NotecardEnvVarCache *)NoteMalloc(sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }
    memset(cache, 0, sizeof(*cache));

    return cache;
}

bool NotecardEnvVarCache_update(NotecardEnvVarCache *cache, const char *var,
                                const char *val)
{
    bool success = true;

    if (cache == NULL || var == NULL || val == NULL) {
        return false;
    }

    NotecardEnvVarCache *entry = cache;
    NotecardEnvVarCache *last = cache;
    bool updated = false;
    while (entry != NULL && entry->var != NULL) {
        last = entry;
        if (strcmp(var, entry->var) == 0) {
            NoteFree(entry->val);
            entry->val = strdup(val);
            if (entry->val == NULL) {
                success = false;
            } else {
                updated = true;
            }

            break;
        }
        entry = entry->next;
    }

    // If no entry in the cache with that variable name was found, add a new
    // entry for the variable onto the end of the list.
    if (!updated) {
        if (last != NULL) {
            success = _addEntry(last, var, val);
        } else {
            // This should never happen.
            success = false;
        }
    }

    return success;
}
