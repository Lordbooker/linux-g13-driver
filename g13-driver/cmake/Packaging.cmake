# =============================================================================
# CPack: .deb- und .rpm-Pakete
#
#   cmake -B build -S g13-driver -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
#   cmake --build build
#   cd build && cpack -G DEB      # bzw. -G RPM
#
# Optionale Variablen (werden im Release-Workflow gesetzt):
#   G13_PACKAGE_VERSION  Paketversion, z. B. 2.1.0 (Standard: aus G13_VERSION)
#   G13_PACKAGE_RELEASE  Paket-Release, z. B. "1~debian13" (Standard: 1)
# =============================================================================

# Paketversion: nur der numerische Teil (2.0.0-3-gabc -> 2.0.0)
if(NOT DEFINED G13_PACKAGE_VERSION)
    string(REGEX MATCH "^[0-9]+(\\.[0-9]+)*" G13_PACKAGE_VERSION "${G13_VERSION}")
    if(NOT G13_PACKAGE_VERSION)
        set(G13_PACKAGE_VERSION "${PROJECT_VERSION}")
    endif()
endif()
if(NOT DEFINED G13_PACKAGE_RELEASE)
    set(G13_PACKAGE_RELEASE "1")
endif()

set(CPACK_PACKAGE_NAME                "linux-g13-driver")
set(CPACK_PACKAGE_VERSION             "${G13_PACKAGE_VERSION}")
set(CPACK_PACKAGE_VENDOR              "Lordbooker")
set(CPACK_PACKAGE_CONTACT             "Booker <lordbooker@gmail.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL        "https://github.com/Lordbooker/linux-g13-driver")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "User-space driver and Qt6 configuration tool for the Logitech G13 gameboard")
set(CPACK_PACKAGE_DESCRIPTION
"User-space driver for the Logitech G13 gameboard (keys, joystick, macros,
LCD and backlight) running as a systemd user service, plus a Qt6 configuration
tool with tray icon. Fork of linux-g13-driver by Jim Gupta.")
set(CPACK_RESOURCE_FILE_LICENSE       "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
set(CPACK_PACKAGING_INSTALL_PREFIX    "${CMAKE_INSTALL_PREFIX}")
set(CPACK_STRIP_FILES                 ON)

# ---------------------------------------------------------------------------
# Debian / Ubuntu
# ---------------------------------------------------------------------------
set(CPACK_DEBIAN_PACKAGE_RELEASE      "${G13_PACKAGE_RELEASE}")
set(CPACK_DEBIAN_FILE_NAME            DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION      "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY     "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS    ON)     # Abhaengigkeiten aus den Binaries
# Qt-Plattform-Plugins (xcb, wayland, ...) liegen bei Debian/Ubuntu in einem
# eigenen Paket, das libqt6gui6 nicht ueberall mitzieht (z. B. Ubuntu 22.04).
# Ohne sie startet g13-gui nicht. Wird mit den shlibdeps zusammengefuehrt.
set(CPACK_DEBIAN_PACKAGE_DEPENDS      "qt6-qpa-plugins")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS   "qt6-wayland")   # native Wayland-Sitzungen
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/deb/postinst"
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/deb/postrm"
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/deb/conffiles")
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)

# ---------------------------------------------------------------------------
# Fedora (RPM)
# ---------------------------------------------------------------------------
set(CPACK_RPM_PACKAGE_RELEASE         "${G13_PACKAGE_RELEASE}")
set(CPACK_RPM_PACKAGE_RELEASE_DIST    ON)     # haengt %{?dist} an, z. B. .fc44
set(CPACK_RPM_FILE_NAME               RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_LICENSE         "GPL-2.0-only")
set(CPACK_RPM_PACKAGE_GROUP           "System Environment/Daemons")
set(CPACK_RPM_PACKAGE_URL             "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_RPM_PACKAGE_DESCRIPTION     "${CPACK_PACKAGE_DESCRIPTION}")
set(CPACK_RPM_PACKAGE_AUTOREQ         ON)     # Abhaengigkeiten aus den Binaries
set(CPACK_RPM_PACKAGE_SUGGESTS        "qt6-qtwayland")  # native Wayland-Sitzungen
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE   "${CMAKE_CURRENT_SOURCE_DIR}/packaging/rpm/post")
set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/rpm/postun")
# Verzeichnisse, die bereits anderen Systempaketen gehoeren, nicht beanspruchen
# (sonst Dateikonflikte mit "filesystem", "systemd", "hicolor-icon-theme" ...).
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    /etc/xdg
    /etc/xdg/autostart
    /usr/lib/udev
    /usr/lib/udev/rules.d
    /usr/lib/systemd
    /usr/lib/systemd/user
    /usr/share/applications
    /usr/share/icons
    /usr/share/icons/hicolor
    /usr/share/icons/hicolor/scalable
    /usr/share/icons/hicolor/scalable/apps
    /usr/share/icons/hicolor/22x22
    /usr/share/icons/hicolor/22x22/apps
    /usr/share/icons/hicolor/32x32
    /usr/share/icons/hicolor/32x32/apps
    /usr/share/icons/hicolor/48x48
    /usr/share/icons/hicolor/48x48/apps
    /usr/share/icons/hicolor/64x64
    /usr/share/icons/hicolor/64x64/apps
    /usr/share/icons/hicolor/128x128
    /usr/share/icons/hicolor/128x128/apps
    /usr/share/icons/hicolor/256x256
    /usr/share/icons/hicolor/256x256/apps)
# Autostart-Eintrag als Konfigurationsdatei (Aenderungen des Admins bleiben erhalten)
set(CPACK_RPM_USER_FILELIST
    "%config(noreplace) /etc/xdg/autostart/g13-gui-tray.desktop"
    "%license /usr/share/doc/linux-g13-driver/LICENSE")

include(CPack)
