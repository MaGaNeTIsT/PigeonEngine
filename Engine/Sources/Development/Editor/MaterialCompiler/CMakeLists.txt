cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(MaterialCompiler VERSION 1.0 LANGUAGES CXX)

# Match engine configuration types
if(NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;DebugEditor;Develop;DevelopEditor;Shipping" CACHE STRING "" FORCE)
endif()

# DXC — headers and lib from project third-party folder.
# Expected layout after you copy the DXC release there:
#   ThirdParty/DirectXShaderCompiler/inc/dxcapi.h
#   ThirdParty/DirectXShaderCompiler/lib/x64/dxcompiler.lib
#   ThirdParty/DirectXShaderCompiler/bin/x64/dxcompiler.dll
#   ThirdParty/DirectXShaderCompiler/bin/x64/dxil.dll
set(DXC_THIRDPARTY_ROOT "${CMAKE_SOURCE_DIR}/Engine/ThirdParty/DirectXShaderCompiler"
    CACHE PATH "Path to DXC inside ThirdParty (contains inc/ lib/ bin/)")

set(DXC_INCLUDE_DIR "${DXC_THIRDPARTY_ROOT}/inc")
set(DXC_LIB_DIR     "${DXC_THIRDPARTY_ROOT}/lib/x64")
set(DXC_BIN_DIR     "${DXC_THIRDPARTY_ROOT}/bin/x64")

add_executable(MaterialCompiler
    Main.cpp
    MCJsonParser.h       MCJsonParser.cpp
    MCTemplateAssembler.h MCTemplateAssembler.cpp
    MCDxcCompiler.h      MCDxcCompiler.cpp
    MCReflectionWriter.h MCReflectionWriter.cpp
    MCFileWatcher.h      MCFileWatcher.cpp
)

set_target_properties(MaterialCompiler PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS ON
    CXX_STANDARD_REQUIRED ON
)

foreach(CONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set_target_properties(MaterialCompiler PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/Build64/Bin/${CONFIG_UPPER}"
        PDB_OUTPUT_DIRECTORY_${CONFIG_UPPER}     "${CMAKE_BINARY_DIR}/Build64/Bin/${CONFIG_UPPER}"
    )
endforeach()

target_compile_options(MaterialCompiler PRIVATE
    $<$<CONFIG:Debug>:/MTd>
    $<$<CONFIG:DebugEditor>:/MTd>
    $<$<CONFIG:Develop>:/MTd>
    $<$<CONFIG:DevelopEditor>:/MTd>
    $<$<CONFIG:Shipping>:/MT>
    $<$<CONFIG:Debug>:/ZI>
    $<$<CONFIG:DebugEditor>:/ZI>
    $<$<CONFIG:Develop>:/Zi>
    $<$<CONFIG:DevelopEditor>:/Zi>
    $<$<CONFIG:Shipping>:/Zi>
)

target_include_directories(MaterialCompiler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${DXC_INCLUDE_DIR}
    "${CMAKE_SOURCE_DIR}/Engine/ThirdParty/rapidjson"
)

target_link_libraries(MaterialCompiler PRIVATE
    "${DXC_LIB_DIR}/dxcompiler.lib"
)

if(MSVC)
    target_compile_definitions(MaterialCompiler PRIVATE
        _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
    )
    target_compile_options(MaterialCompiler PRIVATE
        /nologo
        /diagnostics:classic
        /D_MBCS
        /sdl
        /MP
        /W4
        /WX
        /Gm-
        /EHsc
        /fp:fast
        /fp:except-
        /Zc:wchar_t
        /Zc:forScope
        /Zc:inline
        /std:c++17
        /Gd
        /FC
        /wd4201
        /wd4100
        /wd4238
    )
    target_compile_options(MaterialCompiler PRIVATE
        $<$<CONFIG:Debug>:/Od /Ob0 /RTC1 /GS /Zc:rvalueCast>
        $<$<CONFIG:DebugEditor>:/Od /Ob0 /RTC1 /GS /Zc:rvalueCast>
        $<$<CONFIG:Develop>:/O2 /Oi /Ob1 /GS /Zc:rvalueCast>
        $<$<CONFIG:DevelopEditor>:/O2 /Oi /Ob1 /GS /Zc:rvalueCast>
        $<$<CONFIG:Shipping>:/O2 /Oi /Ot /GL /Ob2 /Gy>
    )
    target_link_options(MaterialCompiler PRIVATE
        $<$<CONFIG:Debug>:/INCREMENTAL /DEBUG>
        $<$<CONFIG:DebugEditor>:/INCREMENTAL /DEBUG>
        $<$<CONFIG:Develop>:/INCREMENTAL /DEBUG>
        $<$<CONFIG:DevelopEditor>:/INCREMENTAL /DEBUG>
        $<$<CONFIG:Shipping>:/INCREMENTAL:NO>
    )
    target_link_options(MaterialCompiler PRIVATE
        $<$<CONFIG:Develop>:/LTCG>
        $<$<CONFIG:DevelopEditor>:/LTCG>
        $<$<CONFIG:Shipping>:/LTCG>
    )
    # Copy dxcompiler.dll + dxil.dll next to the exe after build
    add_custom_command(TARGET MaterialCompiler POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DXC_BIN_DIR}/dxcompiler.dll"
            $<TARGET_FILE_DIR:MaterialCompiler>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DXC_BIN_DIR}/dxil.dll"
            $<TARGET_FILE_DIR:MaterialCompiler>
        COMMENT "Copying DXC runtime DLLs from EngineThirdParty"
    )
    # Run MaterialCompiler on all material sources
    add_custom_command(TARGET MaterialCompiler POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/Build/Temp/MaterialAssets"
        COMMAND $<TARGET_FILE:MaterialCompiler>
            --shader-include "${CMAKE_SOURCE_DIR}/Engine/Sources/EngineCore/Shaders"
            "${CMAKE_SOURCE_DIR}/Assets/MaterialSources"
            "${CMAKE_SOURCE_DIR}/Build/Temp/MaterialAssets"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Compiling materials..."
    )
endif()
