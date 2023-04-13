# Notecard Environment Variable Manager

This is a C library for fetching environment variables from a [Notecard](https://blues.io/products/notecard/).

## Dependencies

This library uses

- [note-c](https://github.com/blues/note-c) to communicate with the Notecard
- [Catch2](https://github.com/catchorg/Catch2) for unit testing.

## Usage

The core of the library is the `NotecardEnvVarManager` object. To create a `NotecardEnvVarManager`, call `NotecardEnvVarManager_alloc`:

```c
NotecardEnvVarManager *manager = NotecardEnvVarManager_alloc();
if (manager == NULL) {
    // Handle failed allocation.
}
```

If successful, `NotecardEnvVarManager_alloc` returns a pointer to a `NotecardEnvVarManager`. This is an opaque struct; users are not intended to access the members of the struct directly. To free manager's memory, call `NotecardEnvVarManager_free`:

```c
NotecardEnvVarManager_free(manager);
```

After allocating a manager, there are two functions for fetching environment variables from the Notecard:

- `NotecardEnvVarManager_fetch`
- `NotecardEnvVarManager_process`

Under the hood, both of these use an [`env.get`](https://dev.blues.io/reference/notecard-api/env-requests/#env-get) request, specifying the  variables to fetch via the `names` field. After getting the variable:value pairs from the Notecard, the manager will call a user-provided callback on each pair. This callback must have the following signature:

```c
typedef void (*envVarCb)(const char *var, const char *val, void *ctx);
```

To set the callback, call `NotecardEnvVarManager_setEnvVarCb`. This function also allows the user to set a pointer to an arbitrary "user context," which will be passed to the callback by the manager. In the example below, the user context is a pointer to a struct used to cache the values of environment variables.

```c
typedef struct {
    int valueA;
    int valueB;
    int valueC;
} EnvVarCache;

void envVarManagerCb(const char *var, const char *val, void *userCtx)
{
    // Cast the userCtx to the appropriate type.
    EnvVarCache *cache = (EnvVarCache *)userCtx;

    if (strcmp(var, "variable_a") == 0) {
        printf("variable_a has value %s\n", val);
        cache->valueA = atoi(val);
    }
    else if (strcmp(var, "variable_b") == 0) {
        printf("variable_b has value %s\n", val);
        cache->valueB = atoi(val);
    }
    else if (strcmp(var, "variable_c") == 0) {
        printf("variable_c has value %s\n", val);
        cache->valueC = atoi(val);
    }
}

int main(void)
{
    EnvVarCache myCache;
    if (NotecardEnvVarManager_setEnvVarCb(manager, envVarManagerCb, &myCache) != NEVM_SUCCESS)
    {
        // Handle failure.
    }
}
```

`NotecardEnvVarManager_setEnvVarCb` returns `NEVM_SUCCESS` on success and `NEVM_FAILURE` on failure.

Note that if an environment variable is requested by the user that doesn't exist, nothing for that variable will be returned by the Notecard, and the user's callback won't be called for that variable. Thus, the user doesn't need to worry about their callback being called with the `var` or `val` parameters set to NULL.

### `NotecardEnvVarManager_fetch`

The simplest way to get variables from the Notecard is by calling `NotecardEnvVarManager_fetch` with an array of C-strings indicating the variables of interest:

```c
const char *vars[] = {
    "variable_a",
    "variable_b",
    "variable_c"
};
const size_t numVars = sizeof(vars) / sizeof(vars[0]);

int ret = NotecardEnvVarManager_fetch(manager, vars, numVars);
if (ret != NEVM_SUCCESS) {
    // Handle failure.
}
```

This function returns `NEVM_SUCCESS` on success and `NEVM_FAILURE` on failure. This function makes an `env.get` request to the Notecard for the specified variables and calls the user-provided callback on each variable:value pair in the response.

### `NotecardEnvVarManager_process`

The other way of getting variables from the Notecard is by calling `NotecardEnvVarManager_process`. This approach uses "watch variables." A watch variable is an environment variable that the user is interested in fetching from the Notecard. Similar to `NotecardEnvVarManager_fetch`, the user sets watch variables by calling `NotecardEnvVarManager_setWatchVars` with an array of C-strings indicating the variables of interest:

```c
const char *watchVars[] = {
    "variable_a",
    "variable_b",
    "variable_c"
};
const size_t numWatchVars = sizeof(watchVars) / sizeof(watchVars[0]);

int ret = NotecardEnvVarManager_setWatchVars(manager, watchVars, numWatchVars);
if (ret != NEVM_SUCCESS) {
    // Handle failure.
}
```

This function returns `NEVM_SUCCESS` on success and `NEVM_FAILURE` on failure.

With watch variables set, the user can fetch them from the Notecard by calling `NotecardEnvVarManager_process`:

```c
int ret = NotecardEnvVarManager_process(manager);
if (ret == NEVM_FAILURE) {
    // Handle failure.
}
else if (ret == NEVM_WAITING) {
    // Watch interval hasn't lapsed.
}
```

`NotecardEnvVarManager_process` returns `NEVM_SUCCESS` on success and `NEVM_FAILURE` on failure.

By default, calling `NotecardEnvVarManager_process` will make an `env.get` request whenever it's called. However, the user can set a watch interval so that calls to `NotecardEnvVarManager_process` will only make the `env.get` request every N seconds, no matter how many times the process function is called within that time farme. The idea is that the user can continually call `NotecardEnvVarManager_process` in a tight loop (think Arduino's `loop` function) without hammering the Notecard with `env.get` requests. To set a watch interval, call `NotecardEnvVarManager_setWatchInterval`:

```c
const uint32_t watchIntervalSeconds = 10;
int ret = NotecardEnvVarManager_setWatchInterval(manager, watchIntervalSeconds);
if (ret == NEVM_FAILURE) {
    // Handle failure.
}
```

This function returns `NEVM_SUCCESS` on success and `NEVM_FAILURE` on failure. In the example above, if the function returns successfully, calls to `NotecardEnvVarManager_process` will only make an `env.get` request once every 10 seconds. If multiple calls to `NotecardEnvVarManager_process` are made within that 10 second time frame, only the first will make the `env.get` request. The subsequent, redundant calls will return `NEVM_WAITING`, indicating that the manager is "waiting" for the watch interval to lapse.

### `NEVM_ENV_VAR_ALL`

Both `NotecardEnvVarManager_fetch` and `NotecardEnvVarManager_setWatchVars` support a special value for the number of variables, `NEVM_ENV_VAR_ALL`. Using this value will cause ALL environment variables to be fetched from the Notecard (i.e. an `env.get` request with no `names` field will be made).

```c
// Fetch all environment variables.
int ret = NotecardEnvVarManager_fetch(manager, NULL, NEVM_ENV_VAR_ALL);
if (ret != NEVM_SUCCESS) {
    // Handle failure.
}
```

```c
// Watch ALL environment variables.
int ret = NotecardEnvVarManager_setWatchVars(manager, NULL, NEVM_ENV_VAR_ALL);
if (ret != NEVM_SUCCESS) {
    // Handle failure.
}

// Fetch all environment variables.
ret = NotecardEnvVarManager_process(manager);
```

## Unit Tests

### Dependencies

- [CMake](https://cmake.org/)
- [Catch2](https://github.com/catchorg/Catch2)
- [lcov](https://github.com/linux-test-project/lcov) (optional; used for coverage reporting)

note-c is downloaded from GitHub as part of the CMake build, so you don't need to download it up front. You can also download and build Catch2 from GitHub instead of installing it up front by adding `-DNEVM_BUILD_CATCH=1` to your `cmake` command.

### Running the Tests

From the root directory, run this script:

```bash
./scripts/run_unit_tests.sh
```

#### Check for Memory Errors

```bash
./scripts/run_unit_tests.sh --memcheck
```

#### Generate Coverage Data

```bash
./scripts/run_unit_tests.sh --coverage
```
