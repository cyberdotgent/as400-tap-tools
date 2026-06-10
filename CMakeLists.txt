cmake_minimum_required(VERSION 3.24)

project(as400_tap_tools
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(wxBUILD_SHARED OFF CACHE BOOL "Build wxWidgets as shared libraries" FORCE)
set(wxBUILD_MONOLITHIC OFF CACHE BOOL "Build wxWidgets as a single library" FORCE)
set(wxBUILD_SAMPLES OFF CACHE STRING "Build wxWidgets samples" FORCE)
set(wxBUILD_TESTS OFF CACHE STRING "Build wxWidgets tests" FORCE)
set(wxBUILD_DEMOS OFF CACHE BOOL "Build wxWidgets demos" FORCE)
set(wxUSE_AUI OFF CACHE BOOL "Use wxAUI" FORCE)
set(wxUSE_HELP OFF CACHE BOOL "Use wxWidgets help subsystem" FORCE)
set(wxUSE_HTML OFF CACHE BOOL "Use wxHTML" FORCE)
set(wxUSE_MEDIACTRL OFF CACHE BOOL "Use wxMediaCtrl" FORCE)
set(wxUSE_MS_HTML_HELP OFF CACHE BOOL "Use MS HTML Help" FORCE)
set(wxUSE_PROPGRID OFF CACHE BOOL "Use wxPropertyGrid" FORCE)
set(wxUSE_RIBBON OFF CACHE BOOL "Use wxRibbon" FORCE)
set(wxUSE_RICHTEXT OFF CACHE BOOL "Use wxRichTextCtrl" FORCE)
set(wxUSE_STC OFF CACHE BOOL "Use wxStyledTextCtrl" FORCE)
set(wxUSE_WEBVIEW OFF CACHE BOOL "Use wxWebView" FORCE)
set(wxUSE_WXHTML_HELP OFF CACHE BOOL "Use wxHTML-based help" FORCE)
set(wxUSE_XRC OFF CACHE BOOL "Use XRC resources" FORCE)

add_library(tap STATIC
    src/tap/Error.h
    src/tap/Reader.cpp
    src/tap/Reader.h
    src/tap/Result.h
    src/tap/Scanner.cpp
    src/tap/Scanner.h
    src/tap/SimhFormat.cpp
    src/tap/SimhFormat.h
    src/tap/TapeElement.cpp
    src/tap/TapeElement.h
    src/tap/TapeImage.cpp
    src/tap/TapeImage.h
    src/tap/Writer.cpp
    src/tap/Writer.h
)

target_include_directories(tap
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_library(as400 STATIC
    src/as400/Cp37.cpp
    src/as400/Cp37.h
    src/as400/FileList.cpp
    src/as400/FileList.h
    src/as400/RecordParser.cpp
    src/as400/RecordParser.h
    src/as400/utils/As400Date.cpp
    src/as400/utils/As400Date.h
    src/as400/utils/FixedWidthText.cpp
    src/as400/utils/FixedWidthText.h
    src/as400/utils/Format.cpp
    src/as400/utils/Format.h
    src/as400/utils/RecordFields.cpp
    src/as400/utils/RecordFields.h
)

target_include_directories(as400
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(as400
    PUBLIC
        tap
)

add_subdirectory(vendor/wxWidgets)

set(APP_ICON_ICO "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/as400-tap-tools.ico")
set(APP_ICON_ICNS "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/as400-tap-tools.icns")
set(APP_ICON_PNG "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/as400-tap-tools.png")

add_executable(as400-tap-tools WIN32 MACOSX_BUNDLE
    src/App.cpp
    src/App.h
    src/As400FileListDialog.cpp
    src/As400FileListDialog.h
    src/DecoderPropertyPane.cpp
    src/DecoderPropertyPane.h
    src/HexFormatter.cpp
    src/HexFormatter.h
    src/MainFrame.cpp
    src/MainFrame.h
    src/TapeProgressDialog.cpp
    src/TapeProgressDialog.h
    src/TapeElementView.cpp
    src/TapeElementView.h
    ${APP_ICON_PNG}
)

if(WIN32)
    target_sources(as400-tap-tools PRIVATE src/windows/app.rc ${APP_ICON_ICO})
endif()

if(APPLE)
    set_target_properties(as400-tap-tools PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "AS400 Tap Tools"
        MACOSX_BUNDLE_GUI_IDENTIFIER "gent.cyber.as400taptools"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "as400-tap-tools.icns"
        RESOURCE "${APP_ICON_ICNS};${APP_ICON_PNG}"
        OUTPUT_NAME "as400-tap-tools"
    )
else()
    add_custom_command(TARGET as400-tap-tools POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:as400-tap-tools>/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${APP_ICON_PNG}"
            "$<TARGET_FILE_DIR:as400-tap-tools>/assets/as400-tap-tools.png"
    )
endif()

target_link_libraries(as400-tap-tools
    PRIVATE
        as400
        tap
        wx::base
        wx::core
)

add_executable(tap-tests
    src/HexFormatter.cpp
    src/HexFormatter.h
    tests/tap/TapTests.cpp
)

target_link_libraries(tap-tests
    PRIVATE
        as400
        tap
)

target_compile_definitions(tap-tests
    PRIVATE
        TAP_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/examples"
)

add_test(NAME tap-tests COMMAND tap-tests)

if(MINGW)
    target_link_options(as400-tap-tools PRIVATE -static -static-libgcc -static-libstdc++)
    target_link_options(tap-tests PRIVATE -static -static-libgcc -static-libstdc++)
endif()
