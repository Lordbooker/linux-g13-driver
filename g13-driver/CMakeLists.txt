cmake_minimum_required(VERSION 3.20)

project(Linux-G13-Driver VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # fuer clang-tidy

# --- Build Options ---
option(BUILD_CONFIG_TOOL "Build the Qt6 config tool" ON)
option(BUILD_TESTS       "Build unit and integration tests" ON)
option(ENABLE_SANITIZERS "Enable ASan/UBSan in debug builds" OFF)

# --- Dependencies (Daemon) ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB  REQUIRED libusb-1.0)
pkg_check_modules(LIBUDEV REQUIRED libudev)

# --- Dependencies (GUI) ---
if(BUILD_CONFIG_TOOL)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)
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
    ${LIBUDEV_INCLUDE_DIRS}
)

target_link_libraries(linux-g13-driver PRIVATE
    ${LIBUSB_LIBRARIES}
    ${LIBUDEV_LIBRARIES}
)

target_compile_options(linux-g13-driver PRIVATE
    ${LIBUSB_CFLAGS_OTHER}
    ${LIBUDEV_CFLAGS_OTHER}
)

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
        src/resources.qrc
    )

    target_include_directories(g13-gui PRIVATE src/gui)

    target_link_libraries(g13-gui PRIVATE
        Qt6::Widgets
        Qt6::Core
        Qt6::Gui
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
endif()
install(FILES src/udev/99-g13.rules
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/udev/rules.d
)
install(FILES src/systemd/g13.service
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/user
)
