cmake_minimum_required(VERSION 3.20)

# The version must match the release tag (checked by the release workflow).
project(Linux-G13-Driver VERSION 2.6.3 LANGUAGES CXX)

# --- Version ------------------------------------------------------------------
# G13_VERSION: displayed version (driver log, GUI). Default: "git describe"
# without the leading "v" (e.g. 2.0.0 or 2.0.0-3-gd1568bb), else PROJECT_VERSION.
if(NOT DEFINED G13_VERSION)
    find_package(Git QUIET)
    if(GIT_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --match "v[0-9]*"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE G13_GIT_DESCRIBE
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
    endif()
    if(G13_GIT_DESCRIBE)
        string(REGEX REPLACE "^v" "" G13_VERSION "${G13_GIT_DESCRIBE}")
    else()
        set(G13_VERSION "${PROJECT_VERSION}")
    endif()
endif()
message(STATUS "linux-g13-driver version: ${G13_VERSION}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # for clang-tidy

# --- Build options ---
option(BUILD_CONFIG_TOOL "Build the Qt6 config tool" ON)
option(ENABLE_SANITIZERS "Enable ASan/UBSan in debug builds" OFF)
option(G13_WERROR        "Treat compiler warnings as errors (used by CI)" OFF)

add_compile_options(-Wall -Wextra -Wpedantic -Wshadow)
if(G13_WERROR)
    add_compile_options(-Werror)
endif()

if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# --- Dependencies ---
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0)

if(BUILD_CONFIG_TOOL)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Network LinguistTools)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
endif()

# =========================================================
# TARGET 1: driver daemon (no GUI libraries)
# =========================================================
add_executable(linux-g13-driver
    src/driver/Main.cpp
    src/driver/G13.cpp
    src/driver/G13Action.cpp
    src/driver/MacroAction.cpp
    src/driver/ConfigPath.cpp
    src/driver/PassThroughAction.cpp
    src/driver/Output.cpp
    src/driver/IntroAnimation.cpp
)
target_include_directories(linux-g13-driver PRIVATE src/driver)
target_link_libraries(linux-g13-driver PRIVATE PkgConfig::LIBUSB Threads::Threads)
target_compile_definitions(linux-g13-driver PRIVATE G13_VERSION="${G13_VERSION}")

# =========================================================
# TARGET 2: Qt6 config tool (only if BUILD_CONFIG_TOOL)
# =========================================================
if(BUILD_CONFIG_TOOL)
    add_executable(g13-gui
        src/gui/main.cpp
        src/gui/MainWindow.cpp
        src/gui/MainWindow.h
        src/gui/MainWindow.ui
        src/gui/G13Visualizer.cpp
        src/gui/G13Visualizer.h
        src/gui/BindingEditor.cpp
        src/gui/BindingEditor.h
        src/gui/ColorSelector.cpp
        src/gui/ColorSelector.h
        src/gui/MacroRecorder.cpp
        src/gui/MacroRecorder.h
        src/gui/MacroSequence.cpp
        src/gui/MacroSequence.h
        src/gui/KeyNames.cpp
        src/gui/KeyCaptureEdit.cpp
        src/gui/KeyCaptureEdit.h
        src/gui/G13Keys.cpp
        src/gui/G13Keys.h
        src/gui/KeyNames.h
        src/gui/PropertiesFile.cpp
        src/gui/PropertiesFile.h
        src/resources.qrc
    )

    target_include_directories(g13-gui PRIVATE src/gui)

    target_link_libraries(g13-gui PRIVATE
        Qt6::Widgets
        Qt6::Core
        Qt6::Gui
        Qt6::Network    # QLocalServer/QLocalSocket for the single instance
    )
    target_compile_definitions(g13-gui PRIVATE G13_VERSION="${G13_VERSION}")

    # Translations: source texts are English (also the fallback language).
    # The .qm files are generated during the build and embedded under :/i18n;
    # g13-gui picks the matching language at startup (QLocale::system()).
    # Update the .ts files after text changes: cmake --build build --target update_translations
    set(G13_GUI_LANGUAGES de fr it es pt)
    set(G13_GUI_TS_FILES)
    foreach(lang IN LISTS G13_GUI_LANGUAGES)
        list(APPEND G13_GUI_TS_FILES translations/g13-gui_${lang}.ts)
    endforeach()
    qt_add_translations(g13-gui
        TS_FILES ${G13_GUI_TS_FILES}
        RESOURCE_PREFIX "/i18n"
        LUPDATE_OPTIONS -no-obsolete -locations none
    )
endif()

# =========================================================
# Install
# =========================================================
include(GNUInstallDirs)
install(TARGETS linux-g13-driver
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(BUILD_CONFIG_TOOL)
    install(TARGETS g13-gui
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    # Application menu
    install(FILES src/desktop/g13-gui.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    # Tray icon at login (XDG autostart; with prefix /usr -> /etc/xdg/autostart)
    install(FILES src/desktop/g13-gui-tray.desktop
        DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/xdg/autostart
    )
    # Icons (hicolor theme)
    install(FILES src/resources/icons/g13-gui.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    )
    foreach(size 22 32 48 64 128 256)
        install(FILES src/resources/icons/g13-gui-${size}.png
            DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps
            RENAME g13-gui.png
        )
    endforeach()
endif()
# Relative destinations (= <prefix>/lib/...): CPack/RPM would automatically mark
# absolute paths as %config, which is wrong for files under /usr/lib.
install(FILES src/udev/70-g13.rules
    DESTINATION lib/udev/rules.d
)
install(FILES src/systemd/g13.service
    DESTINATION lib/systemd/user
)
# Documentation and license
install(FILES ../README.md ../LICENSE ../AUTHORS
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/linux-g13-driver
)

# =========================================================
# Packages (.deb / .rpm) - see cmake/Packaging.cmake
# =========================================================
include(cmake/Packaging.cmake)
