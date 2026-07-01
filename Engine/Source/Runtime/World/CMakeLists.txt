set(MODULE_NAME WindEffects-World)

file(GLOB_RECURSE SOURCE_FILES
    "DefaultScene/*.cpp"
    "DefaultScene/*.hpp"
    "Environment/*.cpp"
    "Environment/*.h"
)

add_library(${MODULE_NAME} SHARED ${SOURCE_FILES})
set_target_properties(${MODULE_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

target_include_directories(${MODULE_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(${MODULE_NAME} PUBLIC
    WindEffects-Core
    WindEffects-Scene
    WindEffects-Renderer
    glm
)
