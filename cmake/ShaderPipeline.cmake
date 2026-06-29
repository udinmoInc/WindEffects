# WindEffects HLSL-first shader pipeline (DXC -> SPIR-V for Vulkan)
# Future: add DXIL (D3D12) and SPIR-V -> MSL (Metal) targets from the same HLSL sources.

set(WE_SHADER_ROOT "${CMAKE_SOURCE_DIR}/Engine/Shaders" CACHE PATH "Root directory for HLSL shader sources")
set(WE_SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/Generated/Shaders" CACHE PATH "Compiled shader bytecode output directory")
set(WE_SHADER_MODEL "6_6" CACHE STRING "HLSL shader model minor version (6_6+)")

file(MAKE_DIRECTORY "${WE_SHADER_OUTPUT_DIR}")

# Windows SDK ships dxc without SPIR-V codegen — prefer DXC builds that support -spirv.
function(we_dxc_supports_spirv DXC_PATH OUT_VAR)
    set(_test_hlsl "${CMAKE_BINARY_DIR}/_we_dxc_spirv_test.hlsl")
    set(_test_spv "${CMAKE_BINARY_DIR}/_we_dxc_spirv_test.spv")
    file(WRITE "${_test_hlsl}" "void VSMain() {}\n")
    execute_process(
        COMMAND "${DXC_PATH}" -spirv -T vs_6_0 -E VSMain -Fo "${_test_spv}" "${_test_hlsl}"
        RESULT_VARIABLE _rc
        OUTPUT_QUIET ERROR_QUIET
    )
    if(_rc EQUAL 0 AND EXISTS "${_test_spv}")
        set(${OUT_VAR} TRUE PARENT_SCOPE)
    else()
        set(${OUT_VAR} FALSE PARENT_SCOPE)
    endif()
endfunction()

set(WE_DXC_CANDIDATES "")
if(DEFINED ENV{LOCALAPPDATA})
    file(GLOB WE_WINGET_DXC "$ENV{LOCALAPPDATA}/Microsoft/WinGet/Packages/Microsoft.DirectX.ShaderCompiler*/bin/x64/dxc.exe")
    list(APPEND WE_DXC_CANDIDATES ${WE_WINGET_DXC})
endif()
if(DEFINED ENV{VULKAN_SDK})
    list(APPEND WE_DXC_CANDIDATES "$ENV{VULKAN_SDK}/Bin/dxc.exe" "$ENV{VULKAN_SDK}/bin/dxc.exe")
endif()
list(APPEND WE_DXC_CANDIDATES
    "C:/Program Files/Microsoft DirectX Shader Compiler/x64/dxc.exe"
)
file(GLOB WE_WINDKITS "C:/Program Files (x86)/Windows Kits/10/bin/*")
list(SORT WE_WINDKITS COMPARE NATURAL ORDER DESCENDING)
foreach(WE_KIT_BIN IN LISTS WE_WINDKITS)
    list(APPEND WE_DXC_CANDIDATES "${WE_KIT_BIN}/x64/dxc.exe")
endforeach()

set(WE_DXC_EXECUTABLE "" CACHE FILEPATH "DXC executable with SPIR-V codegen support")
if(WE_DXC_EXECUTABLE AND EXISTS "${WE_DXC_EXECUTABLE}")
    we_dxc_supports_spirv("${WE_DXC_EXECUTABLE}" _forced_spirv)
    if(NOT _forced_spirv)
        message(WARNING "WE_DXC_EXECUTABLE does not support SPIR-V; searching alternatives...")
        set(WE_DXC_EXECUTABLE "" CACHE FILEPATH "" FORCE)
    endif()
endif()

if(NOT WE_DXC_EXECUTABLE)
    foreach(WE_DXC_CANDIDATE IN LISTS WE_DXC_CANDIDATES)
        if(EXISTS "${WE_DXC_CANDIDATE}")
            we_dxc_supports_spirv("${WE_DXC_CANDIDATE}" _candidate_spirv)
            if(_candidate_spirv)
                set(WE_DXC_EXECUTABLE "${WE_DXC_CANDIDATE}" CACHE FILEPATH "DXC executable with SPIR-V codegen support" FORCE)
                break()
            endif()
        endif()
    endforeach()
endif()

file(GLOB_RECURSE WE_HLSL_INCLUDES CONFIGURE_DEPENDS "${WE_SHADER_ROOT}/*.hlsli")

macro(we_compile_hlsl_shader TARGET_NAME HLSL_SOURCE VS_ENTRY PS_ENTRY)
    set(WE_VS_OUTPUT "${WE_SHADER_OUTPUT_DIR}/${TARGET_NAME}_VS.spv")
    set(WE_PS_OUTPUT "${WE_SHADER_OUTPUT_DIR}/${TARGET_NAME}_PS.spv")

    set(WE_DXC_COMMON_FLAGS
        -spirv
        -HV 2021
        -I "${WE_SHADER_ROOT}"
        -I "${WE_SHADER_ROOT}/Common"
        -fspv-target-env=vulkan1.2
        -Wno-conversion
    )

    if(WE_DXC_EXECUTABLE)
        add_custom_command(
            OUTPUT "${WE_VS_OUTPUT}"
            COMMAND "${WE_DXC_EXECUTABLE}" ${WE_DXC_COMMON_FLAGS}
                -T "vs_${WE_SHADER_MODEL}"
                -E "${VS_ENTRY}"
                -Fo "${WE_VS_OUTPUT}"
                "${HLSL_SOURCE}"
            DEPENDS "${HLSL_SOURCE}" ${WE_HLSL_INCLUDES}
            COMMENT "DXC VS ${TARGET_NAME} <- ${HLSL_SOURCE}"
            VERBATIM
        )
        add_custom_command(
            OUTPUT "${WE_PS_OUTPUT}"
            COMMAND "${WE_DXC_EXECUTABLE}" ${WE_DXC_COMMON_FLAGS}
                -T "ps_${WE_SHADER_MODEL}"
                -E "${PS_ENTRY}"
                -Fo "${WE_PS_OUTPUT}"
                "${HLSL_SOURCE}"
            DEPENDS "${HLSL_SOURCE}" ${WE_HLSL_INCLUDES}
            COMMENT "DXC PS ${TARGET_NAME} <- ${HLSL_SOURCE}"
            VERBATIM
        )
        list(APPEND WE_SHADER_SPV_OUTPUTS "${WE_VS_OUTPUT}" "${WE_PS_OUTPUT}")
    endif()
endmacro()

set(WE_SHADER_SPV_OUTPUTS "")

we_compile_hlsl_shader(EditorBackground "${WE_SHADER_ROOT}/Editor/EditorBackground.hlsl" VSMain PSMain)
we_compile_hlsl_shader(Grid             "${WE_SHADER_ROOT}/Editor/Grid.hlsl"             VSMain PSMain)
we_compile_hlsl_shader(SceneObject       "${WE_SHADER_ROOT}/Rendering/SceneObject.hlsl" VSMain PSMain)
we_compile_hlsl_shader(UI                "${WE_SHADER_ROOT}/Rendering/UI.hlsl"          VSMain PSMain)

if(WE_DXC_EXECUTABLE AND WE_SHADER_SPV_OUTPUTS)
    add_custom_target(WindEffects_CompileShaders DEPENDS ${WE_SHADER_SPV_OUTPUTS})
    message(STATUS "HLSL shader pipeline: DXC=${WE_DXC_EXECUTABLE}")
else()
    add_custom_target(WindEffects_CompileShaders)
    message(WARNING "SPIR-V-capable DXC not found. Install Microsoft.DirectX.ShaderCompiler (winget) or Vulkan SDK. Prebuilt SPIR-V in ${WE_SHADER_OUTPUT_DIR} will be used if present.")
endif()
