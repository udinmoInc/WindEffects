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
endif()

# Set output directory
set_target_properties(${PROGRAM_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Build/Win64/${CMAKE_CFG_INTDIR}"
)
