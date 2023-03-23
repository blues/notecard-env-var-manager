/*!
 * @file NotecardEnvVarManager_test.cpp
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
#include "fff.h"

#include "note-c/note.h"

#include "NotecardEnvVarManager.h"

DEFINE_FFF_GLOBALS
FAKE_VALUE_FUNC(J *, NoteTransaction, J *)
FAKE_VALUE_FUNC(int, _envModified, NotecardEnvVarManager *)

namespace
{

const size_t numWatchVars = 3;
const char *watchVars[numWatchVars] = {
    "var_a",
    "var_b",
    "var_c"
};
const char *vals[] = {
    "val_a",
    "val_b",
    "val_c"
};
uint32_t userCtx = 42;
bool changeCbCalled[numWatchVars] = {false, false, false};

int envChangeCb(const char *var, const char *val, void *ctx)
{
    int ret = NEVM_FAILURE;

    for (size_t i = 0; i < sizeof(watchVars)/sizeof(*watchVars); ++i) {
        if (strcmp(var, watchVars[i]) == 0 && strcmp(val, vals[i]) == 0 &&
                userCtx == *((uint32_t *)ctx)) {
            changeCbCalled[i] = true;
            ret = NEVM_SUCCESS;
            break;
        }
    }

    return ret;
}

TEST_CASE("NotecardEnvVarManager")
{
    RESET_FAKE(NoteTransaction);
    RESET_FAKE(_envModified);

    NoteSetFnDefault(malloc, free, NULL, NULL);
    char rawRsp[128];
    char val[16];

    for (size_t i = 0; i < sizeof(changeCbCalled)/sizeof(*changeCbCalled); ++i) {
        changeCbCalled[i] = false;
    }

    NotecardEnvVarManager *man = NotecardEnvVarManager_alloc();
    REQUIRE(man != NULL);
    CHECK(NotecardEnvVarManager_setWatchVars(man, watchVars, numWatchVars) ==
          NEVM_SUCCESS);
    CHECK(NotecardEnvVarManager_setEnvFetchCb(man, envChangeCb, &userCtx) ==
          NEVM_SUCCESS);

    SECTION("No cache") {
        for (size_t i = 0; i < numWatchVars; ++i) {
            J *rsp = JCreateObject();
            REQUIRE(rsp != NULL);
            JAddStringToObject(rsp, "text", vals[i]);
            NoteTransaction_fake.return_val = rsp;

            REQUIRE(NotecardEnvVarManager_get(man, watchVars[i], val,
                                              sizeof(val)) == NEVM_SUCCESS);
            CHECK(strcmp(val, vals[i]) == 0);
            CHECK(changeCbCalled[i]);
        }
    }

    SECTION("Cache") {
        CHECK(NotecardEnvVarManager_enableCache(man) == NEVM_SUCCESS);

        snprintf(rawRsp, sizeof(rawRsp),
                 "{"
                 "\"body\": {"
                 "\"%s\": \"%s\","
                 "\"%s\": \"%s\","
                 "\"%s\": \"%s\""
                 "}"
                 "}", watchVars[0], vals[0], watchVars[1], vals[1], watchVars[2],
                 vals[2]
                );
        J *rsp = JParse(rawRsp);
        REQUIRE(rsp != NULL);
        NoteTransaction_fake.return_val = rsp;

        int envModifiedRetVals[] = {NEVM_SUCCESS, NEVM_FAILURE};
        SET_RETURN_SEQ(_envModified, envModifiedRetVals, 2);

        for (size_t i = 0; i < numWatchVars; ++i) {
            REQUIRE(NotecardEnvVarManager_get(man, watchVars[i], val,
                                              sizeof(val)) == NEVM_SUCCESS);
            CHECK(strcmp(val, vals[i]) == 0);
            CHECK(changeCbCalled[i]);
        }

        // Only one Notecard request should be made to get the env vars.
        // Subsequent env var queries should hit the cache.
        CHECK(NoteTransaction_fake.call_count == 1);

        // Trying to get a variable that isn't watched should fail.
        val[0] = '\0';
        CHECK(NotecardEnvVarManager_get(man, "bogus", val, sizeof(val))
              != NEVM_SUCCESS);
        CHECK(strlen(val) == 0);
    }

    NotecardEnvVarManager_free(man);
}

}

#endif // NEVM_TEST
