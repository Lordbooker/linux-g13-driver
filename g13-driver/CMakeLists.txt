cmake_minimum_required(VERSION 3.16)

project(Linux-G13-Driver VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Dependencies ---
find_package(libusb-1.0 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(APPIND REQUIRED appindicator3-0.1)

# Qt6 for the GUI
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

# --- Configuration ---
set(CMAKE_AUTOMOC ON) # Handle Meta-Object Compilation
set(CMAKE_AUTORCC ON) # Handle Resources
set(CMAKE_AUTOUIC ON) # Handle UI Files

# =========================================================
# TARGET 1: The Driver (Daemon)
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

# Header paths for the driver
target_include_directories(linux-g13-driver PRIVATE src/driver)

# Link driver libraries
target_link_libraries(linux-g13-driver PRIVATE
    libusb-1.0::libusb-1.0
    ${GTK3_LIBRARIES}
    ${APPIND_LIBRARIES}
)

# C-Flags for GTK
target_compile_options(linux-g13-driver PRIVATE ${GTK3_CFLAGS_OTHER} ${APPIND_CFLAGS_OTHER})
target_include_directories(linux-g13-driver PRIVATE ${GTK3_INCLUDE_DIRS} ${APPIND_INCLUDE_DIRS})


# =========================================================
# TARGET 2: The GUI (Config Tool)
# =========================================================

# Define Resource File
set(GUI_RESOURCES src/resources.qrc)

add_executable(g13-gui
    # Core Files
    src/gui/main.cpp
    src/gui/MainWindow.cpp
    src/gui/MainWindow.h
    src/gui/MainWindow.ui
    
    # Components (Frames)
    src/gui/G13Visualizer.cpp
    src/gui/G13Visualizer.h
    src/gui/BindingEditor.cpp
    src/gui/BindingEditor.h
    src/gui/ColorSelector.cpp
    src/gui/ColorSelector.h
    src/gui/MacroRecorder.cpp
    src/gui/MacroRecorder.h

    # Resources
    ${GUI_RESOURCES}
)

# Include Directories for GUI
target_include_directories(g13-gui PRIVATE src/gui)

# Link Qt libraries
target_link_libraries(g13-gui PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
)