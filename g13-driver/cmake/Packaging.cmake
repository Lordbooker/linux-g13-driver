# =============================================================================
# CPack: .deb and .rpm packages
#
#   cmake -B build -S g13-driver -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
#   cmake --build build
#   cd build && cpack -G DEB      # or -G RPM
#
# Optional variables (set by the release workflow):
#   G13_PACKAGE_VERSION  package version, e.g. 2.1.0 (default: from G13_VERSION)
#   G13_PACKAGE_RELEASE  package release, e.g. "1~debian13" (default: 1)
# =============================================================================

# Package version: numeric part only (2.0.0-3-gabc -> 2.0.0)
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
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS    ON)     # dependencies from the binaries
# On Debian/Ubuntu the Qt platform plugins (xcb, wayland, ...) are in a separate
# package that libqt6gui6 does not pull in everywhere (e.g. Ubuntu 22.04).
# g13-gui does not start without them. Merged with the shlibdeps.
set(CPACK_DEBIAN_PACKAGE_DEPENDS      "qt6-qpa-plugins")
# qt6-wayland: native Wayland sessions; qt6-translations-l10n: Qt's own
# texts (e.g. standard buttons) in the system language.
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS   "qt6-wayland, qt6-translations-l10n")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/deb/postinst"
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/deb/postrm"
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/deb/conffiles")
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)

# ---------------------------------------------------------------------------
# Fedora (RPM)
# ---------------------------------------------------------------------------
set(CPACK_RPM_PACKAGE_RELEASE         "${G13_PACKAGE_RELEASE}")
set(CPACK_RPM_PACKAGE_RELEASE_DIST    ON)     # appends %{?dist}, e.g. .fc44
set(CPACK_RPM_FILE_NAME               RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_LICENSE         "GPL-2.0-only")
set(CPACK_RPM_PACKAGE_GROUP           "System Environment/Daemons")
set(CPACK_RPM_PACKAGE_URL             "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_RPM_PACKAGE_DESCRIPTION     "${CPACK_PACKAGE_DESCRIPTION}")
set(CPACK_RPM_PACKAGE_AUTOREQ         ON)     # dependencies from the binaries
# qt6-qtwayland: native Wayland sessions; qt6-qttranslations: Qt's own texts
set(CPACK_RPM_PACKAGE_SUGGESTS        "qt6-qtwayland, qt6-qttranslations")
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE   "${CMAKE_CURRENT_SOURCE_DIR}/packaging/rpm/post")
set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/rpm/postun")
# Do not claim directories that already belong to other system packages
# (otherwise file conflicts with "filesystem", "systemd", "hicolor-icon-theme" ...).
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
# Autostart entry as configuration file (changes by the admin are preserved)
set(CPACK_RPM_USER_FILELIST
    "%config(noreplace) /etc/xdg/autostart/g13-gui-tray.desktop"
    "%license /usr/share/doc/linux-g13-driver/LICENSE")

include(CPack)
