# Generate favicon_data.h from favicon.ico
set(FAVICON_ICO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/favicon.ico")
set(FAVICON_DATA_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/favicon_data.h")
set(GENERATE_FAVICON_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/generate_favicon_data.py")

# Create custom command to generate favicon_data.h
add_custom_command(
    OUTPUT "${FAVICON_DATA_HEADER}"
    COMMAND python3 "${GENERATE_FAVICON_SCRIPT}" "${FAVICON_ICO_PATH}" "${FAVICON_DATA_HEADER}"
    DEPENDS "${FAVICON_ICO_PATH}"
    COMMENT "Generating favicon_data.h from favicon.ico"
    VERBATIM
)

# Add as header-only dependency
add_custom_target(favicon_data_header ALL DEPENDS "${FAVICON_DATA_HEADER}")

idf_component_register(SRCS "PowerRelay.cpp"
                    INCLUDE_DIRS "."
                    LDFRAGMENTS "")
