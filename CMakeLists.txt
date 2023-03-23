cmake_minimum_required(VERSION 3.13)

if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
    message(FATAL_ERROR "In-source builds are not allowed.
    Please create a build directory and use `cmake ..` inside it.
    NOTE: cmake will now create CMakeCache.txt and CMakeFiles/*.
          You must delete them, or cmake will refuse to work.")
endif()

project(notecard_env_var_manager)

# Automatically ignore CMake build directory.
if(NOT EXISTS ${PROJECT_BINARY_DIR}/.gitignore)
    file(WRITE ${PROJECT_BINARY_DIR}/.gitignore "*")
endif()

option(NEVM_BUILD_TESTS "Build tests." OFF)
option(BUILD_SHARED_LIBS "Build shared libraries instead of static." OFF)
option(NEVM_COVERAGE "Compile for test coverage reporting." OFF)
option(NEVM_MEM_CHECK "Run tests with Valgrind." OFF)

set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

add_subdirectory(${SRC_DIR}/note-c)

add_library(
    notecard_env_var_manager
    ${SRC_DIR}/log.c
    ${SRC_DIR}/NotecardEnvVarCache.c
    ${SRC_DIR}/NotecardEnvVarManager.c
)
target_compile_options(
    notecard_env_var_manager
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
)
target_include_directories(
    notecard_env_var_manager
    PUBLIC
        ${SRC_DIR}
)
target_link_libraries(
    notecard_env_var_manager
    PUBLIC
        note_c
)

if(NEVM_BUILD_TESTS)
    # Including CTest here rather than in test/CMakeLists.txt allows us to run
    # ctest from the root build directory (e.g. build/ instead of build/test/).
    # We also need to set MEMORYCHECK_COMMAND_OPTIONS before including this.
    # See: https://stackoverflow.com/a/60741757
    if(NEVM_MEM_CHECK)
        # Go ahead and make sure we can find valgrind while we're here.
        find_program(VALGRIND valgrind REQUIRED)
        message(STATUS "Found valgrind: ${VALGRIND}")
        set(MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --error-exitcode=1")
    endif(NEVM_MEM_CHECK)
    include(CTest)

    target_compile_definitions(
        notecard_env_var_manager
        PUBLIC NEVM_TEST
    )

    # If we don't weaken the functions we're mocking in the tests, the linker
    # will complain about multiple function definitions: the mocked one and the
    # "real" one from notecard_env_var_manager. Weakening the real function
    # causes the mock function, if defined, to override the real one. If no mock
    # is defined, the real one will be used. So, every time a developer needs to
    # mock a function in a test, they need to make sure it's included in the
    # MOCKED_FNS list below.
    set(
        MOCKED_FNS
        "_envModified"
    )
    foreach(MOCKED_FN ${MOCKED_FNS})
        string(APPEND OBJCOPY_WEAKEN "-W ${MOCKED_FN} ")
    endforeach()
    separate_arguments(OBJCOPY_WEAKEN_LIST NATIVE_COMMAND "${OBJCOPY_WEAKEN}")
    add_custom_command(TARGET notecard_env_var_manager POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_WEAKEN_LIST}
        $<TARGET_FILE:notecard_env_var_manager>
        COMMENT "Weakening mocked functions."
    )

    add_subdirectory(test)
endif(NEVM_BUILD_TESTS)
