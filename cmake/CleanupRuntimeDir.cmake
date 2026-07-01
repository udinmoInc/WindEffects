# Removes CMake build artifacts that must not live in the editor runtime folder.
# Invoked from WindEffectsEditor POST_BUILD via: cmake -P CleanupRuntimeDir.cmake -DRUNTIME_DIR=...

if(NOT RUNTIME_DIR AND DEFINED ENV{WE_RUNTIME_DIR})
    set(RUNTIME_DIR "$ENV{WE_RUNTIME_DIR}")
endif()

if(NOT RUNTIME_DIR)
    message(FATAL_ERROR "CleanupRuntimeDir.cmake requires -DRUNTIME_DIR=... or WE_RUNTIME_DIR env var")
endif()

set(_remove_dirs
    CMakeFiles
    _deps
    Generated
    bin
    Engine
    x64
    ALL_BUILD.dir
    ZERO_CHECK.dir
    WindEffects_CompileShaders.dir
)

foreach(_dir IN LISTS _remove_dirs)
    set(_path "${RUNTIME_DIR}/${_dir}")
    if(EXISTS "${_path}")
        file(REMOVE_RECURSE "${_path}")
    endif()
endforeach()

foreach(_file IN ITEMS CMakeCache.txt cmake_install.cmake lucide.zip)
    set(_path "${RUNTIME_DIR}/${_file}")
    if(EXISTS "${_path}")
        file(REMOVE "${_path}")
    endif()
endforeach()

file(GLOB _remove_files
    "${RUNTIME_DIR}/*.sln"
    "${RUNTIME_DIR}/*.vcxproj"
    "${RUNTIME_DIR}/*.vcxproj.filters"
    "${RUNTIME_DIR}/_we_dxc_spirv_test.*"
)

foreach(_file IN LISTS _remove_files)
    if(EXISTS "${_file}")
        file(REMOVE "${_file}")
    endif()
endforeach()
