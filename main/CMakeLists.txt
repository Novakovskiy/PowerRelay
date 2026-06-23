# Generate favicon_data.h from favicon.ico if it doesn't exist
set(FAVICON_ICO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/favicon.ico")
set(FAVICON_DATA_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/favicon_data.h")
set(GENERATE_FAVICON_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/generate_favicon_data.py")

# Generate favicon_data.h if it doesn't exist
if(NOT EXISTS "${FAVICON_DATA_HEADER}")
    message(STATUS "Generating favicon_data.h from favicon.ico...")
    execute_process(
        COMMAND python3 "${GENERATE_FAVICON_SCRIPT}" "${FAVICON_ICO_PATH}" "${FAVICON_DATA_HEADER}"
        RESULT_VARIABLE FAVICON_RESULT
        OUTPUT_VARIABLE FAVICON_OUTPUT
        ERROR_VARIABLE FAVICON_ERROR
    )

    if(NOT FAVICON_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to generate favicon_data.h: ${FAVICON_ERROR}")
    endif()
    message(STATUS "${FAVICON_OUTPUT}")
endif()

idf_component_register(SRCS "PowerRelay.cpp"
                    INCLUDE_DIRS ".")
