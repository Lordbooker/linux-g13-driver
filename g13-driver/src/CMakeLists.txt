cmake_minimum_required(VERSION 3.10)

# Project Name
project(linux-g13-driver CXX)

# Enforce C++ Standard (C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable warnings (equivalent to -Wall)
add_compile_options(-Wall)

# Use PkgConfig to find system libraries
find_package(PkgConfig REQUIRED)

# Find modules
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
pkg_check_modules(APPINDICATOR REQUIRED appindicator3-0.1)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0) 

# Add include directories
include_directories(
    cpp
    ${LIBUSB_INCLUDE_DIRS}
    ${APPINDICATOR_INCLUDE_DIRS}
    ${GTK3_INCLUDE_DIRS}
)

# Collect all C++ files from the cpp/ subdirectory
# Explicitly adding ConfigPath to ensure it is picked up if GLOB fails somehow,
# but GLOB usually catches it.
file(GLOB SOURCES "cpp/*.cpp")

# Define the executable
add_executable(Linux-G13-Driver ${SOURCES})

# Link libraries
target_link_libraries(Linux-G13-Driver 
    PRIVATE 
    ${LIBUSB_LIBRARIES} 
    ${APPINDICATOR_LIBRARIES} 
    ${GTK3_LIBRARIES}
    pthread  # Threading Support
    stdc++fs # Filesystem Support
)

# Install target for system-wide installation (AUR/Package support)
install(TARGETS Linux-G13-Driver DESTINATION bin)