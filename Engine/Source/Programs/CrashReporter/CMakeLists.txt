set(PROGRAM_NAME WeCrashReporter)

file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.hpp" "*.rc")

add_executable(${PROGRAM_NAME} WIN32 ${SOURCE_FILES})

# Link modules
target_link_libraries(${PROGRAM_NAME} PRIVATE 
    WindEffects-Core
    WindEffects-Application
    SDL3::SDL3
    nlohmann_json::nlohmann_json
)

target_include_directories(${PROGRAM_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/Engine/Source")

# Deploy next to the editor (Editor target is registered after CrashReporter during discovery)
if(TARGET WindEffectsEditor)
    add_custom_command(TARGET ${PROGRAM_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${PROGRAM_NAME}>"
            "$<TARGET_FILE_DIR:WindEffectsEditor>"
        COMMENT "Deploying ${PROGRAM_NAME} next to WindEffectsEditor"
    )
endif()
