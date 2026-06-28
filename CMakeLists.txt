cmake_minimum_required(VERSION 3.24)
project(WindEffects LANGUAGES CXX C)

# C++23 Standard Configuration
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Folder organization in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Configure strict output directories
set(BASE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/Build/Win64")
if(CMAKE_BUILD_TYPE)
    set(BASE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/Build/Win64/${CMAKE_BUILD_TYPE}")
else()
    set(BASE_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/Build/Win64/Debug") # Default to Debug if not specified (though MSVC handles this per config)
endif()

# For multi-config generators like Visual Studio, CMAKE_RUNTIME_OUTPUT_DIRECTORY will automatically append the config (Debug/Release).
# We set the base path here.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Build/Win64")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Build/Win64")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Build/Win64")

# Set MSVC multi-config outputs explicitly to avoid it appending 'Debug' to our path, we want it perfectly aligned or we let it do its thing.
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_SOURCE_DIR}/Build/Win64/${OUTPUTCONFIG}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_SOURCE_DIR}/Build/Win64/${OUTPUTCONFIG}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_SOURCE_DIR}/Build/Win64/${OUTPUTCONFIG}/Intermediate")
endforeach()


# -----------------------------------------------------------------------------
# Dependency Management: FetchContent
# -----------------------------------------------------------------------------
include(FetchContent)

# 1. Vulkan Headers
message(STATUS "Fetching Vulkan-Headers...")
FetchContent_Declare(
    vulkan_headers
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG v1.3.290
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(vulkan_headers)

# 2. Volk (Vulkan Meta-Loader)
message(STATUS "Fetching Volk...")
FetchContent_Declare(
    volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(volk)

# 3. SDL3
message(STATUS "Fetching SDL3...")
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

# 5. GLM
message(STATUS "Fetching GLM...")
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

# 6. Dear ImGui (Docking Branch)
message(STATUS "Fetching Dear ImGui (Docking Branch)...")
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

# Define ImGui Library Target
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC 
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC SDL3::Headers Vulkan::Headers volk)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_NO_PROTOTYPES)

# 7. stb (for stb_truetype.h)
message(STATUS "Fetching stb...")
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
endif()

# 8. NanoSVG (for SVG rasterization)
message(STATUS "Fetching NanoSVG...")
FetchContent_Declare(
    nanosvg
    GIT_REPOSITORY https://github.com/memononen/nanosvg.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(nanosvg)
if(NOT nanosvg_POPULATED)
    FetchContent_Populate(nanosvg)
endif()

# Download Inter fonts
set(FONT_DIR "${CMAKE_BINARY_DIR}/bin/Fonts")
set(FONT_REGULAR "${FONT_DIR}/Inter-Regular.ttf")
set(FONT_BOLD "${FONT_DIR}/Inter-Bold.ttf")

if(NOT EXISTS "${FONT_REGULAR}")
    message(STATUS "Downloading Inter-Regular.ttf...")
    file(DOWNLOAD 
        "https://github.com/rsms/inter/raw/master/docs/font-files/Inter-Regular.ttf"
        "${FONT_REGULAR}"
        SHOW_PROGRESS
    )
endif()
if(NOT EXISTS "${FONT_BOLD}")
    message(STATUS "Downloading Inter-Bold.ttf...")
    file(DOWNLOAD 
        "https://github.com/rsms/inter/raw/master/docs/font-files/Inter-Bold.ttf"
        "${FONT_BOLD}"
        SHOW_PROGRESS
    )
endif()

# Download Lucide Icons
set(ICONS_DIR "${CMAKE_BINARY_DIR}/bin/Icons")
set(LUCIDE_ZIP "${CMAKE_BINARY_DIR}/lucide.zip")
if(NOT EXISTS "${ICONS_DIR}")
    message(STATUS "Downloading Lucide Icons...")
    file(DOWNLOAD 
        "https://github.com/lucide-icons/lucide/releases/download/0.292.0/lucide-icons-0.292.0.zip"
        "${LUCIDE_ZIP}"
        SHOW_PROGRESS
    )
    file(ARCHIVE_EXTRACT 
        INPUT "${LUCIDE_ZIP}"
        DESTINATION "${ICONS_DIR}"
    )
endif()

# -----------------------------------------------------------------------------
# Automatic Module Discovery
# -----------------------------------------------------------------------------
# We search for CMakeLists.txt in the following subdirectories:
set(MODULE_SEARCH_PATHS
    "Engine/Source/Runtime"
    "Engine/Source/Editor"
    "Engine/Source/Developer"
    "Engine/Source/Programs"
    "Engine/Plugins"
)

foreach(SEARCH_PATH IN LISTS MODULE_SEARCH_PATHS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SEARCH_PATH}")
        file(GLOB SUBDIRS LIST_DIRECTORIES true "${CMAKE_CURRENT_SOURCE_DIR}/${SEARCH_PATH}/*")
        foreach(SUBDIR ${SUBDIRS})
            if(IS_DIRECTORY "${SUBDIR}")
                if(EXISTS "${SUBDIR}/CMakeLists.txt")
                    message(STATUS "Discovered Module: ${SUBDIR}")
                    add_subdirectory("${SUBDIR}")
                endif()
            endif()
        endforeach()
    endif()
endforeach()

# -----------------------------------------------------------------------------
# Move Feature Modules to Modules/ Directory
# -----------------------------------------------------------------------------
set(FEATURE_MODULES
    WindEffects-ContentBrowser
    WindEffects-Details
    WindEffects-Docking
    WindEffects-MainFrame
    WindEffects-Menus
    WindEffects-PropertyEditor
    WindEffects-Toolbar
    WindEffects-Viewport
    WindEffects-WorldOutliner
)

foreach(MOD IN LISTS FEATURE_MODULES)
    if(TARGET ${MOD})
        foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
            set_target_properties(${MOD} PROPERTIES 
                RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_SOURCE_DIR}/Build/Win64/${OUTPUTCONFIG}/Modules"
                LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${CMAKE_SOURCE_DIR}/Build/Win64/${OUTPUTCONFIG}/Modules"
            )
        endforeach()
        if(NOT CMAKE_CONFIGURATION_TYPES)
            set_target_properties(${MOD} PROPERTIES 
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Build/Win64/$<CONFIG>/Modules"
                LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Build/Win64/$<CONFIG>/Modules"
            )
        endif()
    endif()
endforeach()
