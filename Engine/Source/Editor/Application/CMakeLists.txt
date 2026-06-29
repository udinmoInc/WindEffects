
set(MODULE_NAME WindEffects-Application)

file(GLOB_RECURSE SOURCE_FILES
    "Private/*.cpp"
    "Private/*.hpp"
)
# Trigger CMake glob refresh

file(GLOB_RECURSE PUBLIC_HEADERS
    "Public/*.hpp"
)

add_library(${MODULE_NAME} SHARED ${SOURCE_FILES} ${PUBLIC_HEADERS})
set_target_properties(${MODULE_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

target_include_directories(${MODULE_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")
target_include_directories(${MODULE_NAME} PUBLIC ${stb_SOURCE_DIR} ${nanosvg_SOURCE_DIR}/src)


target_link_libraries(${MODULE_NAME} PUBLIC 
    WindEffects-Core 
    WindEffects-Renderer
    glm 
    imgui 
    volk 
    SDL3::SDL3
)
