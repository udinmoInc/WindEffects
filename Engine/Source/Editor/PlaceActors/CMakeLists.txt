
set(MODULE_NAME WindEffects-PlaceActors)

file(GLOB_RECURSE SOURCE_FILES
    "Private/*.cpp"
)

file(GLOB_RECURSE PUBLIC_HEADERS
    "Public/*.h"
)

add_library(${MODULE_NAME} SHARED ${SOURCE_FILES} ${PUBLIC_HEADERS})
set_target_properties(${MODULE_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

target_include_directories(${MODULE_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")

target_link_libraries(${MODULE_NAME} PUBLIC
    WindEffects-Core
    WindEffects-Application
    WindEffects-Engine
    WindEffects-Scene
    WindEffects-Toolbar
    WindEffects-ContentBrowser
    glm
    SDL3::SDL3
)
