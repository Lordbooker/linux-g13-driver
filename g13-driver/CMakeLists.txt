cmake_minimum_required(VERSION 3.20)

project(Linux-G13-Driver VERSION 2.0.0 LANGUAGES CXX)

# --- Versionsangabe -----------------------------------------------------------
# G13_VERSION: Anzeige-Version (Treiber-Log, GUI). Standard: "git describe" ohne
# fuehrendes "v" (z. B. 2.0.0 oder 2.0.0-3-gd1568bb), sonst PROJECT_VERSION.
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
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # fuer clang-tidy

# --- Build Options ---
option(BUILD_CONFIG_TOOL "Build the Qt6 config tool" ON)
option(BUILD_TESTS       "Build unit and integration tests" OFF)
option(ENABLE_SANITIZERS "Enable ASan/UBSan in debug builds" OFF)

# --- Dependencies (Daemon) ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB  REQUIRED libusb-1.0)

# --- Dependencies (GUI) ---
if(BUILD_CONFIG_TOOL)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Network)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
endif()

# --- Dependencies (Tests) ---
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
endif()

# --- Sanitizer-Support (nur Debug) ---
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# =========================================================
# TARGET 1: Treiber-Daemon (kein GUI-Link)
# =========================================================
add_executable(linux-g13-driver
    src/driver/Main.cpp
    src/driver/G13.cpp
    src/driver/G13Action.cpp
    src/driver/Macro.cpp
    src/driver/MacroAction.cpp
    src/driver/ConfigPath.cpp
    src/driver/PassThroughAction.cpp
    src/driver/Output.cpp
)

target_include_directories(linux-g13-driver PRIVATE
    src/driver
    ${LIBUSB_INCLUDE_DIRS}
)

target_link_libraries(linux-g13-driver PRIVATE
    ${LIBUSB_LIBRARIES}
)

target_compile_options(linux-g13-driver PRIVATE
    ${LIBUSB_CFLAGS_OTHER}
)
target_compile_definitions(linux-g13-driver PRIVATE G13_VERSION="${G13_VERSION}")

# =========================================================
# TARGET 2: Qt6 Config-Tool (nur wenn BUILD_CONFIG_TOOL)
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
        src/gui/G13Keys.cpp
        src/gui/G13Keys.h
        src/gui/KeyNames.h
        src/resources.qrc
    )

    target_include_directories(g13-gui PRIVATE src/gui)

    target_link_libraries(g13-gui PRIVATE
        Qt6::Widgets
        Qt6::Core
        Qt6::Gui
        Qt6::Network    # QLocalServer/QLocalSocket fuer die Einzelinstanz
    )
    target_compile_definitions(g13-gui PRIVATE G13_VERSION="${G13_VERSION}")
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
    # Anwendungsmenue
    install(FILES src/desktop/g13-gui.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    # Tray-Symbol beim Anmelden (XDG-Autostart; bei Prefix /usr -> /etc/xdg/autostart)
    install(FILES src/desktop/g13-gui-tray.desktop
        DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/xdg/autostart
    )
    # Icons (hicolor-Theme)
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
# Relative Ziele (= <prefix>/lib/...): absolute Pfade wuerden von CPack/RPM
# automatisch als %config markiert, was fuer Dateien unter /usr/lib falsch ist.
install(FILES src/udev/99-g13.rules
    DESTINATION lib/udev/rules.d
)
install(FILES src/systemd/g13.service
    DESTINATION lib/systemd/user
)
# Dokumentation und Lizenz
install(FILES ../README.md ../LICENSE ../AUTHORS
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/linux-g13-driver
)

# =========================================================
# Pakete (.deb / .rpm) – siehe cmake/Packaging.cmake
# =========================================================
include(cmake/Packaging.cmake)
