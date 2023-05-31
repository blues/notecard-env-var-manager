#include <Notecard.h>

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
#define FETCH_INTERVAL_MS (20 * 1000)

// A struct to cache the values of environment variables.
typedef struct {
    char valueA[16];
    char valueB[16];
    char valueC[16];
} EnvVarCache;

Notecard notecard;
NotecardEnvVarManager *envVarManager = NULL;
EnvVarCache envVarCache;
uint32_t lastFetchMs = 0;

void envVarManagerCb(const char *var, const char *val, void *userCtx)
{
    EnvVarCache *cache = (EnvVarCache *)userCtx;

    // Cache the values for each variable.
    if (strcmp(var, "variable_a") == 0) {
        strncpy(cache->valueA, val, (sizeof(cache->valueA) - sizeof('\0')));
    }
    else if (strcmp(var, "variable_b") == 0) {
        strncpy(cache->valueB, val, (sizeof(cache->valueB) - sizeof('\0')));
    }
    else if (strcmp(var, "variable_c") == 0) {
        strncpy(cache->valueC, val, (sizeof(cache->valueC) - sizeof('\0')));
    }
}

// These are the environment variables we'll be fetching from the Notecard.
const char *envVars[] = {
    "variable_a",
    "variable_b",
    "variable_c"
};
static const size_t numEnvVars = sizeof(envVars) / sizeof(envVars[0]);

static bool failure = false;

void setup()
{
    Serial.begin(115200);
    // Wait for serial to be ready for up to 3 seconds.
    const size_t serialTimeoutMs = 3000;
    for (unsigned long startMs = millis(); !Serial
         && (millis() - startMs) < serialTimeoutMs;);

    if (!Serial) {
        failure = true;
        return;
    }

    // Set up debug output via serial connection.
    notecard.setDebugOutputStream(Serial);

    // Initialize the physical I/O channel to the Notecard.
    notecard.begin();

    // Issue the hub.set request to set the ProductUID on the Notecard.
    J *req = notecard.newRequest("hub.set");
    if (PRODUCT_UID[0]) {
       JAddStringToObject(req, "product", PRODUCT_UID);
    }
    JAddStringToObject(req, "mode", "continuous");
    JAddBoolToObject(req, "sync", true);
    // Send the request with a retry timeout. If the Notecard has just started
    // up, it may need a moment before it's able to receive and respond to
    // requests.
    if (!notecard.sendRequestWithRetry(req, HUB_SET_RETRY_SECONDS)) {
        Serial.println("hub.set request failed.");
        failure = true;
        return;
    }

    // Allocate the environment variable manager.
    envVarManager = NotecardEnvVarManager_alloc();
    if (envVarManager == NULL) {
        Serial.println("Failed to allocate env var manager.");
        failure = true;
        return;
    }

    // Set the callback for the manager, and give it a pointer to our cache
    // so that we can store the values of the environment variables we're
    // fetching.
    if (NotecardEnvVarManager_setEnvVarCb(envVarManager, envVarManagerCb,
        &envVarCache) != NEVM_SUCCESS) {
        Serial.println("Failed to set env var manager callback.");
        failure = true;
        return;
    }
}

void loop()
{
    if (failure) {
        if (Serial) {
            Serial.println("Program failed. Halting main loop functionality.");
        }

        delay(10000);
        return;
    }

    // Fetch the environment variables every FETCH_INTERVAL_MS milliseconds.
    unsigned long currentMs = millis();
    if (currentMs - lastFetchMs >= FETCH_INTERVAL_MS) {
        lastFetchMs = currentMs;
        notecard.logDebug("Fetch interval lapsed. Fetching environment "
            "variables...\n");
        if (NotecardEnvVarManager_fetch(envVarManager, envVars, numEnvVars)
            != NEVM_SUCCESS) {
            Serial.println("NotecardEnvVarManager_fetch failed.");
        }

        Serial.print("variable_a has value ");
        Serial.println(envVarCache.valueA);
        Serial.print("variable_b has value ");
        Serial.println(envVarCache.valueB);
        Serial.print("variable_c has value ");
        Serial.println(envVarCache.valueC);
    }
}
