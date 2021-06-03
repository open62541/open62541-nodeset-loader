find_program(LCOV lcov)

if (LCOV)
    # Final tracefile for code coverage
    set(OUTPUT_FILE "coverage.info")

    # Directory where to generate the HTML report
    # set(REPORT_DIR "codecov-report")


    # The baseline run needs to run before tests to learn what zero
    # coverage looks like
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMENT "Generating code coverage base file"
        COMMAND ${LCOV}
        --capture
        --no-external # Do not include external source files
        --base-directory ${CMAKE_SOURCE_DIR}
        --directory ${CMAKE_BINARY_DIR}
        --output-file ${OUTPUT_FILE}
        DEPENDS NodesetLoader)

    add_custom_target(coverage
    DEPENDS
    ${OUTPUT_FILE})
endif()