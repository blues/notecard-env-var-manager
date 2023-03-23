#pragma once

// If we're building the tests, NEVM_STATIC is defined to nothing. This allows
// the tests to access the static functions in the library. Among other things,
// this let's us mock these normally static functions.
#ifdef NEVM_TEST
#define NEVM_STATIC
#else
#define NEVM_STATIC static
#endif

enum {
    NEVM_FAILURE = -1,
    NEVM_SUCCESS = 0,
};
