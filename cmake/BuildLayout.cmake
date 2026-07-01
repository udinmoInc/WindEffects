# WindEffects build layout: keep CMake artifacts out of the runtime output tree.

set(WE_RUNTIME_ROOT "${CMAKE_SOURCE_DIR}/Build/Win64" CACHE INTERNAL "Per-config editor runtime output root")
set(WE_BUILD_STAGING_DIR "${CMAKE_SOURCE_DIR}/Build/cmake" CACHE INTERNAL "CMake binary + staging directory")

function(we_assert_separate_binary_dir)
    file(TO_CMAKE_PATH "${WE_RUNTIME_ROOT}" _runtime_root)
    file(TO_CMAKE_PATH "${CMAKE_BINARY_DIR}" _binary_dir)
    string(TOLOWER "${_runtime_root}" _runtime_lower)
    string(TOLOWER "${_binary_dir}" _binary_lower)

    if(_binary_lower STREQUAL _runtime_lower OR _binary_lower MATCHES "^${_runtime_lower}/")
        message(FATAL_ERROR
            "CMake binary directory must not be inside the runtime output tree.\n"
            "  Runtime: ${WE_RUNTIME_ROOT}\n"
            "  Binary:  ${CMAKE_BINARY_DIR}\n"
            "Reconfigure with:\n"
            "  cmake -S \"${CMAKE_SOURCE_DIR}\" -B \"${WE_BUILD_STAGING_DIR}\"")
    endif()

    file(TO_CMAKE_PATH "${WE_BUILD_STAGING_DIR}" _staging_dir)
    string(TOLOWER "${_staging_dir}" _staging_lower)
    if(NOT _binary_lower STREQUAL _staging_lower)
        message(WARNING
            "Unexpected CMake binary directory.\n"
            "  Current:     ${CMAKE_BINARY_DIR}\n"
            "  Recommended: ${WE_BUILD_STAGING_DIR}\n"
            "Use: cmake -S \"${CMAKE_SOURCE_DIR}\" -B \"${WE_BUILD_STAGING_DIR}\"")
    endif()
endfunction()
