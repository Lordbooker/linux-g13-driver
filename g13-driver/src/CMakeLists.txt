cmake_minimum_required(VERSION 3.16)

project(Linux-G13-Driver VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Dependencies ---
# 1. Treiber Abhängigkeiten
find_package(libusb-1.0 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(APPIND REQUIRED appindicator3-0.1)

# 2. GUI Abhängigkeiten (Qt6)
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

# --- Konfiguration für Qt ---
set(CMAKE_AUTOMOC ON) # Automatische Meta-Object Compilation
set(CMAKE_AUTORCC ON) # Ressourcen
set(CMAKE_AUTOUIC ON) # UI Files (.ui) in C++ Header wandeln

# =========================================================
# TARGET 1: Der Treiber (Daemon)
# =========================================================
# (Liste hier deine bestehenden .cpp Dateien auf. Pfade evtl. anpassen!)
add_executable(linux-g13-driver
    driver/Main.cpp
    driver/G13.cpp
    driver/G13Action.cpp
    driver/Macro.cpp
    driver/MacroAction.cpp
    driver/ConfigPath.cpp
    driver/PassThroughAction.cpp
    driver/Output.cpp
)

# Header Pfade für den Treiber
target_include_directories(linux-g13-driver PRIVATE driver)

# Linken der Treiber-Libraries
target_link_libraries(linux-g13-driver PRIVATE
    libusb-1.0::libusb-1.0
    ${GTK3_LIBRARIES}
    ${APPIND_LIBRARIES}
)

# C-Flags für GTK (wichtig!)
target_compile_options(linux-g13-driver PRIVATE ${GTK3_CFLAGS_OTHER} ${APPIND_CFLAGS_OTHER})
target_include_directories(linux-g13-driver PRIVATE ${GTK3_INCLUDE_DIRS} ${APPIND_INCLUDE_DIRS})


# =========================================================
# TARGET 2: Die GUI (Config Tool)
# =========================================================
add_executable(g13-gui
    gui/main.cpp
    gui/MainWindow.cpp
    gui/MainWindow.h
    # gui/MainWindow.ui  <-- Sobald du eins hast, hier einkommentieren
)

# Linken der Qt-Libraries
target_link_libraries(g13-gui PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
)

# Da wir "g13-gui" als Binary bauen, brauchen wir kein Wrapper-Skript mehr!