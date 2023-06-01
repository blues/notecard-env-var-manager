#include <string.h>
#include <zephyr/kernel.h>
#include "note-c/note.h"
#include "note_c_hooks.h"
#include "NotecardEnvVarManager.h"

// Uncomment this line and replace com.your-company:your-product-name with your
// ProductUID.
// #define PRODUCT_UID "com.your-company:your-product-name"

#ifndef PRODUCT_UID
#define PRODUCT_UID ""
#pragma message "PRODUCT_UID is not defined in this example. Please ensure your Notecard has a product identifier set before running this example or define it in code here. More details at https://bit.ly/product-uid"
#endif

// 5 second timeout for retrying the hub.set request.
#define HUB_SET_RETRY_SECONDS 5
// Fetch every 20 seconds.
#define FETCH_INTERVAL_SECONDS 20

// A struct to cache the values of environment variables.
typedef struct {
    char valueA[16];
    char valueB[16];
    char valueC[16];
} EnvVarCache;

NotecardEnvVarManager *envVarManager = NULL;
EnvVarCache envVarCache;

// These are the environment variables we'll be fetching from the Notecard.
const char *envVars[] = {
    "variable_a",
    "variable_b",
    "variable_c"
};
static const size_t numEnvVars = sizeof(envVars) / sizeof(envVars[0]);

struct k_work envUpdateWorkItem;
struct k_timer envUpdateTimer;

// Callback that will be executed when the environment variable update timer
// expires.
static void envUpdateTimerCb(struct k_timer *timer)
{
    k_work_submit(&envUpdateWorkItem);
}

// Callback that will be executed by the system workqueue when it's time to
// check for environment variable updates.
static void envUpdateWorkCb(struct k_work *item)
{
    NotecardEnvVarManager_fetch(envVarManager, envVars, numEnvVars);
}

void envVarManagerCb(const char *var, const char *val, void *userCtx)
{
    EnvVarCache *cache = (EnvVarCache *)userCtx;

    printk("\nCallback received variable \"%s\" with value \"%s\" and context "
           "%p.\n", var, val, userCtx);

    // Cache the values for each variable.
    if (strcmp(var, "variable_a") == 0) {
        strlcpy(cache->valueA, val, sizeof(cache->valueA));
    } else if (strcmp(var, "variable_b") == 0) {
        strlcpy(cache->valueB, val, sizeof(cache->valueB));
    } else if (strcmp(var, "variable_c") == 0) {
        strlcpy(cache->valueC, val, sizeof(cache->valueC));
    }

    printk("Cached values:\n");
    printk("- variable_a has value %s\n", envVarCache.valueA);
    printk("- variable_b has value %s\n", envVarCache.valueB);
    printk("- variable_c has value %s\n", envVarCache.valueC);
}

int main(void)
{
    // Initialize note-c references.
    NoteSetFnDefault(malloc, free, platform_delay, platform_millis);
    NoteSetFnDebugOutput(note_log_print);
    NoteSetFnI2C(NOTE_I2C_ADDR_DEFAULT, NOTE_I2C_MAX_DEFAULT, note_i2c_reset,
                 note_i2c_transmit, note_i2c_receive);

    k_work_init(&envUpdateWorkItem, envUpdateWorkCb);
    k_timer_init(&envUpdateTimer, envUpdateTimerCb, NULL);

    // Issue the hub.set request to set the ProductUID on the Notecard.
    J *req = NoteNewRequest("hub.set");
    if (PRODUCT_UID[0]) {
        JAddStringToObject(req, "product", PRODUCT_UID);
    }
    JAddStringToObject(req, "mode", "continuous");
    JAddBoolToObject(req, "sync", true);
    JAddStringToObject(req, "sn", "zephyr-env-var-manager");
    // Send the request with a retry timeout. If the Notecard has just started
    // up, it may need a moment before it's able to receive and respond to
    // requests.
    if (!NoteRequestWithRetry(req, HUB_SET_RETRY_SECONDS)) {
        printk("hub.set failed, aborting.\n");
        return -1;
    }

    // Allocate the environment variable manager.
    envVarManager = NotecardEnvVarManager_alloc();
    if (envVarManager == NULL) {
        printk("Failed to allocate env var manager.\n");
        return -1;
    }

    // Set the callback for the manager, and give it a pointer to our cache
    // so that we can store the values of the environment variables we're
    // fetching.
    if (NotecardEnvVarManager_setEnvVarCb(envVarManager, envVarManagerCb,
                                          &envVarCache) != NEVM_SUCCESS) {
        printk("Failed to set env var manager callback.\n");
        return -1;
    }

    k_timer_start(&envUpdateTimer, K_SECONDS(0),
                  K_SECONDS(FETCH_INTERVAL_SECONDS));

    return 0;
}
