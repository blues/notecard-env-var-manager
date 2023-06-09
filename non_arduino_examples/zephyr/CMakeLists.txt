cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(notecard_env_var_manager_zephyr_example)

set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR})
set(NEVM_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/../../src)
set(NOTE_C_DIR ${CMAKE_CURRENT_LIST_DIR}/note-c)
set(NOTE_ZEPHYR_DIR ${CMAKE_CURRENT_LIST_DIR}/note-zephyr/src)

target_sources(
    app
    PRIVATE
        # note-c sources.
        ${NOTE_C_DIR}/n_atof.c
        ${NOTE_C_DIR}/n_cjson.c
        ${NOTE_C_DIR}/n_const.c
        ${NOTE_C_DIR}/n_helpers.c
        ${NOTE_C_DIR}/n_i2c.c
        ${NOTE_C_DIR}/n_printf.c
        ${NOTE_C_DIR}/n_serial.c
        ${NOTE_C_DIR}/n_ua.c
        ${NOTE_C_DIR}/n_b64.c
        ${NOTE_C_DIR}/n_cjson_helpers.c
        ${NOTE_C_DIR}/n_ftoa.c
        ${NOTE_C_DIR}/n_hooks.c
        ${NOTE_C_DIR}/n_md5.c
        ${NOTE_C_DIR}/n_request.c
        ${NOTE_C_DIR}/n_str.c
        # notecard_env_var_manager sources.
        ${NEVM_SRC_DIR}/NotecardEnvVarManager.c
        # note-zephyr sources.
        ${NOTE_ZEPHYR_DIR}/note_c_hooks.c
        # Application sources.
        ${SRC_DIR}/main.c
)
target_include_directories(
    app
    PRIVATE
        ${SRC_DIR}
        ${NEVM_SRC_DIR}
        ${NOTE_ZEPHYR_DIR}
)
