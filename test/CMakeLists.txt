include(FetchContent)

# Catch2 v3 requires C++14.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(BUILD_CATCH "Fetch and build Catch2 from source." OFF)

macro(download_catch2)
    message(STATUS "Downloading Catch2...")
    FetchContent_Declare(
      Catch2
      GIT_REPOSITORY https://github.com/catchorg/Catch2.git
      GIT_TAG        v3.2.0
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endmacro(download_catch2)

if(BUILD_CATCH)
    download_catch2()
else()
    find_package(Catch2 3)

    if(Catch2_FOUND)
        message(STATUS "Found Catch2: ${Catch2_CONFIG}")
    else()
        message(FATAL_ERROR "Couldn't find installed Catch2. To download Catch2 and build from source, re-run cmake with -DBUILD_CATCH=1.")
    endif(Catch2_FOUND)
endif(BUILD_CATCH)

include(Catch)

set(TEST_TARGETS "")

macro(add_test TEST_NAME)
    add_executable(
        ${TEST_NAME}
        ${CMAKE_CURRENT_LIST_DIR}/src/${TEST_NAME}.cpp
    )
    target_include_directories(${TEST_NAME}
        PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include
        PRIVATE ${NOTE_C_SRC_DIR}
    )
    target_link_libraries(${TEST_NAME}
        PRIVATE notecard_env_var_manager
        PRIVATE Catch2::Catch2WithMain
    )

    list(APPEND TEST_TARGETS ${TEST_NAME})

    catch_discover_tests(${TEST_NAME})
endmacro(add_test)

add_test(NotecardEnvVarManager_test)

if(NEVM_COVERAGE)
    find_program(LCOV lcov REQUIRED)
    message(STATUS "Found lcov: ${LCOV}")

    target_compile_options(
        notecard_env_var_manager
        PUBLIC --coverage -g -O0
    )
    target_link_libraries(
        notecard_env_var_manager
        PUBLIC gcov
    )

    # Create the coverage directory and run ctest.
    add_custom_target(
        coverage
        COMMAND mkdir -p coverage
        COMMAND ${CMAKE_CTEST_COMMAND}
        WORKING_DIRECTORY ${CMAKE_CURENT_BINARY_DIR}
    )
    # We don't care about coverage for the tests themselves.
    string(APPEND LCOV_EXCLUDE "--exclude '*/test/*' ")
    # Nor do we care about note-c.
    string(APPEND LCOV_EXCLUDE "--exclude '*/note-c/*' ")
    separate_arguments(LCOV_EXCLUDE_LIST NATIVE_COMMAND "${LCOV_EXCLUDE}")
    # Run lcov to produce a coverage report in the coverage directory.
    add_custom_command(
        TARGET coverage POST_BUILD
        COMMAND lcov --capture --no-external --directory ${ROOT_DIR} ${LCOV_EXCLUDE_LIST} --rc lcov_branch_coverage=1 --output-file lcov.info
        COMMAND lcov --summary --rc lcov_branch_coverage=1 lcov.info
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/coverage
    )
    # The tests have to be built before we can generate the coverage report.
    add_dependencies(coverage ${TEST_TARGETS})
    # Delete the coverage directory when make clean is run.
    set_property(
        DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
        ${CMAKE_CURRENT_BINARY_DIR}/coverage
    )
endif(NEVM_COVERAGE)
