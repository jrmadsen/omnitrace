# adds a ctest for executable
function(OMNITRACE_ADD_BIN_TEST)
    cmake_parse_arguments(
        TEST
        "" # options
        "NAME;TARGET;TIMEOUT;WORKING_DIRECTORY" # single value args
        "ARGS;ENVIRONMENT;LABELS;PROPERTIES;PASS_REGULAR_EXPRESSION;FAIL_REGULAR_EXPRESSION;SKIP_REGULAR_EXPRESSION;DEPENDS;COMMAND" # multiple
        # value args
        ${ARGN})

    if(NOT OMNITRACE_DYNINST_API_RT_DIR AND OMNITRACE_DYNINST_API_RT)
        get_filename_component(OMNITRACE_DYNINST_API_RT_DIR "${OMNITRACE_DYNINST_API_RT}"
                               DIRECTORY)
    endif()

    if(OMNITRACE_BUILD_DYNINST)
        set(OMNITRACE_DYNINST_API_RT_DIR
            "${PROJECT_BINARY_DIR}/external/dyninst/dyninstAPI_RT:${PROJECT_BINARY_DIR}/external/dyninst/dyninstAPI"
            )
    endif()

    if(NOT TEST_ENVIRONMENT)
        set(TEST_ENVIRONMENT
            "OMNITRACE_USE_PERFETTO=ON"
            "OMNITRACE_USE_TIMEMORY=ON"
            "OMNITRACE_USE_SAMPLING=ON"
            "OMNITRACE_TIME_OUTPUT=OFF"
            "LD_LIBRARY_PATH=${PROJECT_BINARY_DIR}:${OMNITRACE_DYNINST_API_RT_DIR}:$ENV{LD_LIBRARY_PATH}"
            )
    endif()

    list(APPEND TEST_ENVIRONMENT "OMNITRACE_CI=ON"
         "OMNITRACE_OUTPUT_PATH=omnitrace-tests-output"
         "OMNITRACE_OUTPUT_PREFIX=${TEST_NAME}/")

    if(TEST_COMMAND)
        add_test(
            NAME ${TEST_NAME}
            COMMAND ${TEST_COMMAND} ${TEST_ARGS}
            WORKING_DIRECTORY ${TEST_WORKING_DIRECTORY})

        set_tests_properties(
            ${TEST_NAME}
            PROPERTIES ENVIRONMENT
                       "${TEST_ENVIRONMENT}"
                       TIMEOUT
                       ${TEST_TIMEOUT}
                       LABELS
                       "omnitrace-bin;${TEST_LABELS}"
                       PASS_REGULAR_EXPRESSION
                       "${TEST_PASS_REGULAR_EXPRESSION}"
                       FAIL_REGULAR_EXPRESSION
                       "${TEST_FAIL_REGULAR_EXPRESSION}"
                       SKIP_REGULAR_EXPRESSION
                       "${TEST_SKIP_REGULAR_EXPRESSION}"
                       ${TEST_PROPERTIES})
    elseif(TARGET ${TEST_TARGET})
        add_test(
            NAME ${TEST_NAME}
            COMMAND $<TARGET_FILE:${TEST_TARGET}> ${TEST_ARGS}
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${TEST_TARGET}>)

        set_tests_properties(
            ${TEST_NAME}
            PROPERTIES ENVIRONMENT
                       "${TEST_ENVIRONMENT}"
                       TIMEOUT
                       ${TEST_TIMEOUT}
                       LABELS
                       "omnitrace-bin;${TEST_LABELS}"
                       PASS_REGULAR_EXPRESSION
                       "${TEST_PASS_REGULAR_EXPRESSION}"
                       FAIL_REGULAR_EXPRESSION
                       "${TEST_FAIL_REGULAR_EXPRESSION}"
                       SKIP_REGULAR_EXPRESSION
                       "${TEST_SKIP_REGULAR_EXPRESSION}"
                       ${TEST_PROPERTIES})
    elseif(OMNITRACE_BUILD_TESTING)
        message(FATAL_ERROR "Error! ${TEST_TARGET} does not exist")
    endif()
endfunction()

omnitrace_add_bin_test(
    NAME omnitrace-exe-help
    TARGET omnitrace-exe
    ARGS --help
    LABELS omnitrace-exe
    TIMEOUT 15
    PASS_REGULAR_EXPRESSION
        ".*\\\[omnitrace\\\] Usage:.*\\\[DEBUG OPTIONS\\\].*\\\[MODE OPTIONS\\\].*\\\[LIBRARY OPTIONS\\\].*\\\[SYMBOL SELECTION OPTIONS\\\].*\\\[RUNTIME OPTIONS\\\].*\\\[GRANULARITY OPTIONS\\\].*\\\[DYNINST OPTIONS\\\].*"
    )

omnitrace_add_bin_test(
    NAME omnitrace-exe-simulate-ls
    TARGET omnitrace-exe
    ARGS --simulate --print-format json txt xml -- ls
    TIMEOUT 60)

omnitrace_add_bin_test(
    NAME omnitrace-exe-simulate-ls-check
    DEPENDS omnitrace-exe-simulate-ls
    COMMAND ls
    WORKING_DIRECTORY
        ${PROJECT_BINARY_DIR}/omnitrace-tests-output/omnitrace-exe-simulate-ls
    TIMEOUT 30
    PASS_REGULAR_EXPRESSION
        ".*available-instr.json.*available-instr.txt.*available-instr.xml.*excluded-instr.json.*excluded-instr.txt.*excluded-instr.xml.*instrumented-instr.json.*instrumented-instr.txt.*instrumented-instr.xml.*overlapping-instr.json.*overlapping-instr.txt.*overlapping-instr.xml.*"
    )

omnitrace_add_bin_test(
    NAME omnitrace-avail-help
    TARGET omnitrace-avail
    ARGS --help
    LABELS omnitrace-avail
    TIMEOUT 15
    PASS_REGULAR_EXPRESSION
        ".*\\\[omnitrace-avail\\\] Usage:.*\\\[CATEGORIES\\\].*\\\[VIEW OPTIONS\\\].*\\\[COLUMN OPTIONS\\\].*\\\[WIDTH OPTIONS\\\].*\\\[OUTPUT OPTIONS\\\].*"
    )

omnitrace_add_bin_test(
    NAME omnitrace-avail-filter-wall-clock-available
    TARGET omnitrace-avail
    ARGS -r wall_clock -C --available
    LABELS omnitrace-avail
    TIMEOUT 15
    PASS_REGULAR_EXPRESSION
        "\\\|[-]+\\\|\n\\\|[ ]+COMPONENT[ ]+\\\|\n\\\|[-]+\\\|\n\\\| (wall_clock)[ ]+\\\|\n\\\| (sampling_wall_clock)[ ]+\\\|\n\\\|[-]+\\\|"
    )

omnitrace_add_bin_test(
    NAME omnitrace-avail-category-filter-omnitrace
    TARGET omnitrace-avail
    ARGS --categories settings::omnitrace --brief
    LABELS omnitrace-avail
    TIMEOUT 15
    PASS_REGULAR_EXPRESSION "OMNITRACE_(SETTINGS_DESC|OUTPUT_FILE|OUTPUT_PREFIX)"
    FAIL_REGULAR_EXPRESSION
        "OMNITRACE_(ADD_SECONDARY|SCIENTIFIC|PRECISION|MEMORY_PRECISION|TIMING_PRECISION)"
    )

omnitrace_add_bin_test(
    NAME omnitrace-avail-category-filter-timemory
    TARGET omnitrace-avail
    ARGS --categories settings::timemory --brief
    LABELS omnitrace-avail
    TIMEOUT 15
    PASS_REGULAR_EXPRESSION
        "OMNITRACE_(ADD_SECONDARY|SCIENTIFIC|PRECISION|MEMORY_PRECISION|TIMING_PRECISION)"
    FAIL_REGULAR_EXPRESSION "OMNITRACE_(SETTINGS_DESC|OUTPUT_FILE)")