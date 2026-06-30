
set(MODULE_NAME WindEffects-EditorGridRenderer)

file(GLOB_RECURSE SOURCE_FILES
    "Private/*.cpp"
    "Private/*.hpp"
)

file(GLOB_RECURSE PUBLIC_HEADERS
    "Public/*.hpp"
)

# Static library: Vulkan entry points are initialized in the editor executable.
# A separate DLL would carry its own volk copy and crash on null vk* pointers.
add_library(${MODULE_NAME} STATIC ${SOURCE_FILES} ${PUBLIC_HEADERS})

target_include_directories(${MODULE_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")

target_link_libraries(${MODULE_NAME} PUBLIC
    WindEffects-Core
    WindEffects-Engine
    WindEffects-Renderer
    glm
    volk
    SDL3::SDL3
)

add_dependencies(${MODULE_NAME} WindEffects_CompileShaders)
