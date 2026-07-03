set(MODULE_NAME WindEffects-Core)

file(GLOB_RECURSE SOURCE_FILES
    "*.cpp"
    "Private/*.cpp"
    "Private/*.hpp"
)

file(GLOB_RECURSE PUBLIC_HEADERS
    "Public/*.hpp"
)

# Add PCH
list(APPEND SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/../RuntimePCH.cpp"
)

add_library(${MODULE_NAME} SHARED ${SOURCE_FILES} ${PUBLIC_HEADERS})
set_target_properties(${MODULE_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

# Use target-based APIs
target_include_directories(${MODULE_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Public>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(${MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_features(${MODULE_NAME} PUBLIC cxx_std_23)

# Setup PCH
we_setup_pch(${MODULE_NAME} "RuntimePCH.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../RuntimePCH.cpp")

target_link_libraries(${MODULE_NAME} PUBLIC
    glm
    volk
    SDL3::SDL3
    dbghelp
    nlohmann_json::nlohmann_json
)

# Unity build
we_setup_unity_build(${MODULE_NAME})
