# -----------------------------------------------------------------------------
# Build Optimizations Module
# Production-grade CMake optimizations for large codebases
# Inspired by Unreal Engine, LLVM, Chromium, and other AAA projects
# -----------------------------------------------------------------------------

option(WE_ENABLE_UNITY_BUILD "Enable unity/jumbo builds for faster compilation" OFF)
option(WE_ENABLE_COMPILER_CACHE "Enable compiler cache (ccache/sccache) if available" ON)
option(WE_ENABLE_BUILD_TIMING "Enable detailed build timing reports" ON)
option(WE_ENABLE_FAST_LINK "Enable fast linking for development builds" ON)

# -----------------------------------------------------------------------------
# Compiler Cache Detection (ccache/sccache)
# -----------------------------------------------------------------------------
if(WE_ENABLE_COMPILER_CACHE)
    find_program(CCACHE_PROGRAM ccache)
    find_program(SCCACHE_PROGRAM sccache)
    
    if(CCACHE_PROGRAM)
        message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    elseif(SCCACHE_PROGRAM)
        message(STATUS "Found sccache: ${SCCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
    else()
        message(STATUS "Compiler cache not found (ccache/sccache)")
    endif()
endif()

# -----------------------------------------------------------------------------
# Build Timing
# -----------------------------------------------------------------------------
if(WE_ENABLE_BUILD_TIMING)
    # Enable CMake build timing
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    
    # Add build timing to CMake configure
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.20)
        cmake_path(SET CMAKE_PROJECT_DIR "${CMAKE_SOURCE_DIR}")
        set(CMAKE_PROJECT_HOMEPAGE_URL "https://github.com/yourusername/windeffects")
    endif()
endif()

# -----------------------------------------------------------------------------
# MSVC Optimizations
# -----------------------------------------------------------------------------
if(MSVC)
    # Enable multi-processor compilation
    add_compile_options(/MP)
    
    # Fast link for development builds
    if(WE_ENABLE_FAST_LINK AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
        add_link_options(/INCREMENTAL)
        if(MSVC_VERSION GREATER_EQUAL 1910)
            add_link_options(/FASTLINK)
        endif()
    endif()
    
    # Enable parallel linking (MSVC 2022+)
    if(MSVC_VERSION GREATER_EQUAL 1930)
        add_link_options(/PARALLEL)
    endif()
    
    # Warning level (warnings as errors disabled for now due to third-party code)
    add_compile_options(/W4)
    
    # Disable specific warnings for third-party code
    add_compile_options(/wd4996) # Disable deprecated warnings
    add_compile_options(/wd4244) # Disable conversion warnings for third-party
    add_compile_options(/wd4456) # Disable local variable hiding
    
    # Enable string pooling
    add_compile_options(/GF)
    
    # Enable intrinsic functions
    add_compile_options(/Oi)
    
    # Favor fast code
    add_compile_options(/Ot)
    
    # Enable function-level linking
    add_compile_options(/Gy)
endif()

# -----------------------------------------------------------------------------
# GCC/Clang Optimizations
# -----------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Enable link-time optimization for release builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-flto)
        add_link_options(-flto)
    endif()
    
    # Enable parallel compilation with make/ninja
    # This is handled by --parallel flag
    
    # Warning level
    add_compile_options(-Wall -Wextra -Wpedantic)
    
    # Treat warnings as errors
    add_compile_options(-Werror)
    
    # Enable color output
    add_compile_options(-fcolor-diagnostics)
endif()

# -----------------------------------------------------------------------------
# Unity Build Support
# -----------------------------------------------------------------------------
function(we_setup_unity_build TARGET_NAME)
    if(WE_ENABLE_UNITY_BUILD)
        # Get source files
        get_target_property(SOURCES ${TARGET_NAME} SOURCES)
        
        # Filter out large files (>1000 lines) from unity build
        set(UNITY_SOURCES "")
        set(SINGLE_SOURCES "")
        
        foreach(SOURCE ${SOURCES})
            if(SOURCE MATCHES "\\.cpp$")
                # Check file size (approximate by line count)
                file(READ "${SOURCE}" CONTENT)
                string(REGEX MATCHALL "\n" NEWLINES "${CONTENT}")
                list(LENGTH NEWLINES LINE_COUNT)
                
                if(LINE_COUNT LESS 1000)
                    list(APPEND UNITY_SOURCES "${SOURCE}")
                else()
                    list(APPEND SINGLE_SOURCES "${SOURCE}")
                endif()
            else()
                list(APPEND SINGLE_SOURCES "${SOURCE}")
            endif()
        endforeach()
        
        # Create unity source file
        if(UNITY_SOURCES)
            set(UNITY_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_Unity.cpp")
            file(WRITE "${UNITY_FILE}" "// Auto-generated unity build file\n")
            
            foreach(SOURCE ${UNITY_SOURCES})
                file(APPEND "${UNITY_FILE}" "#include \"${SOURCE}\"\n")
            endforeach()
            
            # Replace sources with unity file + single files
            set_target_properties(${TARGET_NAME} PROPERTIES
                SOURCES "${UNITY_FILE};${SINGLE_SOURCES}"
            )
            
            message(STATUS "Unity build enabled for ${TARGET_NAME} (${CMAKE_LIST_LENGTH(UNITY_SOURCES)} files)")
        endif()
    endif()
endfunction()

# -----------------------------------------------------------------------------
# Precompiled Headers Setup
# -----------------------------------------------------------------------------
function(we_setup_pch TARGET_NAME PCH_HEADER PCH_SOURCE)
    if(MSVC)
        target_precompile_headers(${TARGET_NAME} PRIVATE
            "<${PCH_HEADER}>"
            "${PCH_SOURCE}"
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_precompile_headers(${TARGET_NAME} PRIVATE
            "<${PCH_HEADER}>"
        )
    endif()
endfunction()

# -----------------------------------------------------------------------------
# Target-Based API Helper
# -----------------------------------------------------------------------------
function(we_setup_module TARGET_NAME)
    # Standard include directories
    target_include_directories(${TARGET_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Public>
        $<INSTALL_INTERFACE:include>
    )
    
    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
    )
    
    # Set C++ standard
    target_compile_features(${TARGET_NAME} PUBLIC cxx_std_23)
    
    # Set visibility
    if(WIN32)
        set_target_properties(${TARGET_NAME} PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS ON
        )
    endif()
    
    # Unity build
    we_setup_unity_build(${TARGET_NAME})
endfunction()

# -----------------------------------------------------------------------------
# Build Configuration Variants
# -----------------------------------------------------------------------------
if(CMAKE_CONFIGURATION_TYPES)
    # Multi-config generator (Visual Studio, Xcode)
    list(APPEND CMAKE_CONFIGURATION_TYPES "Dev" "Profile")
    list(REMOVE_DUPLICATES CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING "" FORCE)
endif()

# Set compiler flags for each configuration
if(MSVC)
    # Development - Fastest compile, minimal optimization
    set(CMAKE_CXX_FLAGS_DEV "/Od /MDd /Zi")
    set(CMAKE_C_FLAGS_DEV "/Od /MDd /Zi")
    
    # Debug - Maximum debugging
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /MDd /Zi /RTC1")
    set(CMAKE_C_FLAGS_DEBUG "/Od /MDd /Zi /RTC1")
    
    # Profile - Optimized with profiling
    set(CMAKE_CXX_FLAGS_PROFILE "/O2 /MD /Zi /DEBUG:FASTLINK")
    set(CMAKE_C_FLAGS_PROFILE "/O2 /MD /Zi /DEBUG:FASTLINK")
    
    # Shipping - Maximum optimization
    set(CMAKE_CXX_FLAGS_SHIPPING "/O2 /MD /GL")
    set(CMAKE_C_FLAGS_SHIPPING "/O2 /MD /GL")
    set(CMAKE_EXE_LINKER_FLAGS_SHIPPING "/LTCG")
    set(CMAKE_SHARED_LINKER_FLAGS_SHIPPING "/LTCG")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Development
    set(CMAKE_CXX_FLAGS_DEV "-O0 -g -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS_DEV "-O0 -g -fno-omit-frame-pointer")
    
    # Debug
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -fno-omit-frame-pointer -fsanitize=address")
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g3 -fno-omit-frame-pointer -fsanitize=address")
    
    # Profile
    set(CMAKE_CXX_FLAGS_PROFILE "-O2 -g -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS_PROFILE "-O2 -g -fno-omit-frame-pointer")
    
    # Shipping
    set(CMAKE_CXX_FLAGS_SHIPPING "-O3 -DNDEBUG")
    set(CMAKE_C_FLAGS_SHIPPING "-O3 -DNDEBUG")
endif()

# -----------------------------------------------------------------------------
# Dependency Analysis
# -----------------------------------------------------------------------------
function(we_analyze_dependencies TARGET_NAME)
    if(WE_ENABLE_BUILD_TIMING)
        # Generate include dependency graph
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Analyzing dependencies for ${TARGET_NAME}..."
            COMMAND ${CMAKE_CXX_COMPILER} -MM ${SOURCES} -MF ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.deps
            COMMENT "Generating dependency graph for ${TARGET_NAME}"
        )
    endif()
endfunction()

# -----------------------------------------------------------------------------
# Parallel Build Configuration
# -----------------------------------------------------------------------------
if(NOT DEFINED CMAKE_JOB_POOLS)
    # Use all available cores for parallel builds
    include(ProcessorCount)
    ProcessorCount(NPROC)
    
    if(NPROC GREATER 0)
        set(CMAKE_JOB_POOLS "compile=${NPROC}" "link=2")
        message(STATUS "Parallel build configured with ${NPROC} compile jobs")
    endif()
endif()

# -----------------------------------------------------------------------------
# Output Build Configuration Summary
# -----------------------------------------------------------------------------
message(STATUS "======================================")
message(STATUS "Build Configuration Summary")
message(STATUS "======================================")
message(STATUS "Unity Build: ${WE_ENABLE_UNITY_BUILD}")
message(STATUS "Compiler Cache: ${WE_ENABLE_COMPILER_CACHE}")
message(STATUS "Build Timing: ${WE_ENABLE_BUILD_TIMING}")
message(STATUS "Fast Link: ${WE_ENABLE_FAST_LINK}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "======================================")
