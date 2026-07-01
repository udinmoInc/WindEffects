
set(MODULE_NAME WindEffects-ContentBrowser)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    "Private/*.cpp"
    "Private/*.hpp"
)

list(APPEND SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Private/Services/ContentBrowserFolderArt.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Private/Services/ContentBrowserBlueprintArt.cpp"
)
list(REMOVE_DUPLICATES SOURCE_FILES)

file(GLOB_RECURSE PUBLIC_HEADERS
    "Public/*.hpp"
)

add_library(${MODULE_NAME} SHARED ${SOURCE_FILES} ${PUBLIC_HEADERS})
set_target_properties(${MODULE_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

target_include_directories(${MODULE_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")


target_link_libraries(${MODULE_NAME} PUBLIC 
    WindEffects-Core
    WindEffects-Application 
    WindEffects-Renderer
    glm 
    volk 
    SDL3::SDL3
)

add_dependencies(${MODULE_NAME} WindEffects-Application)

target_include_directories(${MODULE_NAME} PRIVATE ${stb_SOURCE_DIR} ${nanosvg_SOURCE_DIR}/src)
