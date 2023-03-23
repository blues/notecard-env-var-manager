/*!
 * @file NotecardEnvVarCache_test.cpp
 *
 * Written by the Blues Inc. team.
 *
 * Copyright (c) 2023 Blues Inc. MIT License. Use of this source code is
 * governed by licenses granted by the copyright holder including that found in
 * the
 * <a href="https://github.com/blues/note-c/blob/master/LICENSE">LICENSE</a>
 * file.
 *
 */

#ifdef NEVM_TEST

#include <catch2/catch_test_macros.hpp>
#include <cstring>

#include "note-c/note.h"

#include "NotecardEnvVarCache.h"

namespace
{

TEST_CASE("NotecardEnvVarCache")
{
    NoteSetFnDefault(malloc, free, NULL, NULL);

    NotecardEnvVarCache *cache = NotecardEnvVarCache_new();
    REQUIRE(cache != NULL);

    const char *vars[] = {
        "var_a",
        "var_b",
        "var_c"
    };
    const char *vals[] = {
        "val_a",
        "val_b",
        "val_c"
    };

    // Add all the variable:value pairs to the cache.
    for (size_t i = 0; i < sizeof(vars)/sizeof(*vars); ++i) {
        REQUIRE(NotecardEnvVarCache_update(cache, vars[i], vals[i]));
    }

    // Make sure we can successfully retrieve everything we just added.
    const char *retVal;
    for (size_t i = 0; i < sizeof(vars)/sizeof(*vars); ++i) {
        retVal = NotecardEnvVarCache_get(cache, vars[i]);
        REQUIRE(retVal != NULL);
        CHECK(strcmp(retVal, vals[i]) == 0);
    }

    // Make sure getting an unknown variable fails.
    CHECK(NotecardEnvVarCache_get(cache, "bogus") == NULL);

    // Make sure we can update an existing variable and retrieve the new value.
    const char *newVal = "new_val";
    REQUIRE(NotecardEnvVarCache_update(cache, vars[0], newVal));
    retVal = NotecardEnvVarCache_get(cache, vars[0]);
    REQUIRE(retVal != NULL);
    CHECK(strcmp(retVal, newVal) == 0);

    NotecardEnvVarCache_free(cache);
}

}

#endif // NEVM_TEST
