set(PROGRAM_NAME IgniteBT)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    "*.cpp"
    "*.hpp"
)

add_executable(${PROGRAM_NAME} ${SOURCE_FILES})

# Use target-based APIs
target_include_directories(${PROGRAM_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(${PROGRAM_NAME} PUBLIC cxx_std_23)

# Link dependencies
target_link_libraries(${PROGRAM_NAME} PRIVATE
    nlohmann_json::nlohmann_json
)

# Platform-specific linking
if(WIN32)
    target_link_libraries(${PROGRAM_NAME} PRIVATE shell32)
    # Try C++20 instead of C++23 to avoid Windows SDK compatibility issues
    target_compile_features(${PROGRAM_NAME} PUBLIC cxx_std_20)
    target_compile_options(${PROGRAM_NAME} PRIVATE
        "/DNOMINMAX"
        "/DWIN32_LEAN_AND_MEAN"
        "/D_CRT_SECURE_NO_WARNINGS"
        "/D_CRT_NONSTDC_NO_WARNINGS"
    )
endif()

# Set output directory
set_target_properties(${PROGRAM_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Build/Win64/${CMAKE_CFG_INTDIR}"
)
