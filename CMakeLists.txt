cmake_minimum_required(VERSION 3.21)

project(CustomCar VERSION 1.0.1)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
set(CMAKE_GENERATOR_TOOLSET "v142" CACHE STRING "Toolset version") # 142 = RL's version

# set output directory to ./plugins
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/plugins")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})  # for .dll (executables)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})  # for .dll (if seen as a shared lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})  # for .lib and .pdb (static or debug info)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(compile_options
    /std:c++20
    /EHsc
    /MP

    /Zi # generate debug info
    /O2 # optimize
    /bigobj # handle the massive rlsdk files
)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(BMSDK_DIR "${EXTERNAL_DIR}/BMSDK")
set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external")

set(BOILERPLATE_DIR "${SRC_DIR}/boilerplate")
set(BOILERPLATE_SRC
    ${BOILERPLATE_DIR}/GuiBase.cpp
    ${BOILERPLATE_DIR}/PluginHelperBase.cpp
    ${BOILERPLATE_DIR}/${PROJECT_NAME}.rc
)

set(UTIL_DIR "${SRC_DIR}/util")
set(UTIL_SRC
    ${UTIL_DIR}/Instances.cpp
    ${UTIL_DIR}/HookManager.cpp
    ${UTIL_DIR}/FNameCache.cpp
)

set(COMPONENTS_DIR "${SRC_DIR}/components")
set(COMPONENTS_SRC
    ${COMPONENTS_DIR}/CustomCars.cpp
    ${COMPONENTS_DIR}/CustomToppers.cpp
    ${COMPONENTS_DIR}/GameCars.cpp
)


set(RLSDK_DIR "${EXTERNAL_DIR}/RLSDK/RLSDK_w_pch_includes")
set(RLSDK_SRC
    ${RLSDK_DIR}/GameDefines.cpp
    # ${RLSDK_DIR}/SDK_HEADERS/AkAudio_classes.cpp
    ${RLSDK_DIR}/SDK_HEADERS/Core_classes.cpp
    ${RLSDK_DIR}/SDK_HEADERS/Engine_classes.cpp
    ${RLSDK_DIR}/SDK_HEADERS/Extras.cpp
    # ${RLSDK_DIR}/SDK_HEADERS/GFxUI_classes.cpp
    # ${RLSDK_DIR}/SDK_HEADERS/IpDrv_classes.cpp
    # ${RLSDK_DIR}/SDK_HEADERS/OnlineSubsystemEOS_classes.cpp
    ${RLSDK_DIR}/SDK_HEADERS/ProjectX_classes.cpp
    ${RLSDK_DIR}/SDK_HEADERS/TAGame_classes.cpp
    # ${RLSDK_DIR}/SDK_HEADERS/WinDrv_classes.cpp
    # ${RLSDK_DIR}/SDK_HEADERS/XAudio2_classes.cpp
)

set(IMGUI_DIR "${EXTERNAL_DIR}/BakkesmodPluginTemplate/IMGUI")
set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imguivariouscontrols.cpp
    ${IMGUI_DIR}/imgui_additions.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_impl_dx11.cpp
    ${IMGUI_DIR}/imgui_impl_win32.cpp
    ${IMGUI_DIR}/imgui_rangeslider.cpp
    ${IMGUI_DIR}/imgui_searchablecombo.cpp
    ${IMGUI_DIR}/imgui_stdlib.cpp
    ${IMGUI_DIR}/imgui_timeline.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
)

set(MODUTILS_DIR "${EXTERNAL_DIR}/ModUtils")
set(MODUTILS_SRC
    ${MODUTILS_DIR}/gui/GuiTools.cpp
    ${MODUTILS_DIR}/util/Utils.cpp
)

set(CUSTOM_DEFINES "" CACHE STRING "Semicolon-separated list of compile definitions")

if(CUSTOM_DEFINES)
    add_compile_definitions(${CUSTOM_DEFINES})
endif()

add_link_options(/DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF)

add_compile_options(${compile_options})
add_compile_definitions(UNICODE _UNICODE)

if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR
    "This project requires a 64-bit build.\n"
    "Make sure your toolchain is configured to use a 64-bit compiler."
  )
endif()

add_library(${PROJECT_NAME}
    SHARED
    ${IMGUI_SRC}
    ${RLSDK_SRC}
    ${MODUTILS_SRC}
    ${BOILERPLATE_SRC}
    ${UTIL_SRC}
    ${COMPONENTS_SRC}
    ${SRC_DIR}/${PROJECT_NAME}.cpp
    ${SRC_DIR}/SettingsGUI.cpp
    ${SRC_DIR}/pch.cpp
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE ${SRC_DIR}/pch.h
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/src
    PRIVATE ${CMAKE_SOURCE_DIR}/external
    PRIVATE ${CMAKE_SOURCE_DIR}/external/BMSDK/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE ${CMAKE_SOURCE_DIR}/external/BMSDK/lib/pluginsdk.lib
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS OFF
)

if(NOT "$ENV{CI}" STREQUAL "true")
    set(INSTALLED_BMSDK_DIR "$ENV{APPDATA}/bakkesmod/bakkesmod/bakkesmodsdk" CACHE PATH "Installed BakkesMod SDK directory")

    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND "${INSTALLED_BMSDK_DIR}/bakkesmod-patch.exe" "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "Running bakkesmod-patch.exe after building ${PROJECT_NAME}"
    )
endif()
