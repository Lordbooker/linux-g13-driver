# Linux G13 Driver – Development Roadmap

This roadmap describes the development path from the original prototype to a stable,
packaged open source solution. Status: version 2.6.

---

## Phase 1 – Core driver (completed)

**Goal:** A working daemon that passes G13 keys on to the system via `uinput`.

| Task | Status |
|---|---|
| USB communication via `libusb-1.0` | Done |
| Virtual keyboard via `uinput` | Done |
| Reading binding files (`.properties`) | Done |
| Joystick emulation (key mode) | Done |
| Basic LCD functions (buffer, pixels, text) | Done |
| Four binding profiles (switched with L1–L4) with live reload | Done |
| Macro support | Done |
| LCD FIFO interface (`$XDG_RUNTIME_DIR/g13-lcd`, fallback `/tmp/g13-lcd`) | Done |
| `systemd` user unit + `udev` rule | Done |

---

## Phase 2 – Refactoring & robustness (completed)

**Goal:** Daemon without a GUI toolkit, modern build system, stable behavior on errors.

| Task | Status |
|---|---|
| C++ standard **C++23**, CMake **3.20+** | Done |
| **GTK3 / AppIndicator removed from the daemon** – pure background service | Done |
| Build options `BUILD_CONFIG_TOOL`, `ENABLE_SANITIZERS`, `G13_WERROR`; `GNUInstallDirs` | Done |
| Warning-free build (`-Wall -Wextra -Wpedantic -Wshadow`, `-Werror` in CI) | Done |
| Clean shutdown: the signal handler only sets a flag, all device threads are joined | Done |
| Bindings are reset before every load (keys not listed have no function) | Done |
| No stuck keys on profile switch/live reload; profile switch only on key press | Done |
| Retry after a failed open; finished device threads are cleaned up | Done |
| USB read errors: pause, throttled logging, release and reopen after a series of errors | Done |
| Invalid `color` values, init errors and LCD character 127 no longer cause crashes/UB | Done |
| In-kernel driver (`hid-lg-g15`, Linux 6.19+) is reattached when the driver stops | Done |
| Macros: no lost stop on a quick release, keys released on stop, delays end immediately | Done |
| uinput: all keyboard key codes, no joystick axes; held keys released when a device goes away | Done |
| LCD pipe only accepted if it belongs to the user; no configuration fallback in `/tmp` | Done |
| Code review with clang-tidy and cppcheck: no findings | Done |

---

## Phase 3 – Qt6 config tool (completed, v2.0)

**Goal:** Graphical configuration based on **Qt6** as a replacement for the Java tool.

### 3.1 Architecture

```
linux-g13-driver  (systemd user service, no GUI toolkit)
        ^
        |  configuration files ~/.config/g13/*.properties
        |  (driver: live reload of the bindings file of the active profile)
        |
g13-gui           (Qt6 application, tray icon via XDG autostart)
   ├── MainWindow      – main window, tray menu, single instance
   ├── G13Visualizer   – clickable G13 image
   ├── BindingEditor   – key bindings per profile
   ├── ColorSelector   – backlight (RGB)
   └── MacroRecorder   – record and edit macros
```

GUI and driver communicate only through the configuration files; there is no direct
interface (IPC). The tray menu controls the driver via `systemctl --user`.

### 3.2 Implemented

| Task | Status |
|---|---|
| Clickable G13 image (undistorted, scaling) incl. M1–M3/MR, BD, thumb keys, joystick directions | Done |
| Highlighting of bound keys and of the active profile (L1–L4) | Done |
| Binding editor: key code, macro (once / while held / n times), no action | Done |
| Color selection per profile | Done |
| Macro editor: recording with delays, event list, manual insertion, fixed delay | Done |
| Tray icon: open configuration, start/stop/restart driver, status | Done |
| XDG autostart of the tray icon, menu entry, own app icon | Done |
| Single instance: starting again opens the existing window | Done |
| Translations based on the system language (Qt Linguist): English as source/fallback, German, French, Italian, Spanish, Portuguese | Done |
| Driver log messages consistently in English | Done |
| Key input by pressing the key instead of a selection list, key names in the user's own layout | Done |
| LCD start animation (flame wall, Tux and logo), can be disabled under Options | Done |
| Permissions: `uaccess` only (rule `70-g13.rules`, no `MODE="0666"`), LCD pipe 0600 | Done |
| Profile files written atomically, live reload with nanoseconds; live reload also after the file is first created | Done |
| `make` only builds; dependencies via `make deps` | Done |

---

## Phase 4 – Open issues and extended features

### 4.1 Known limitations

| Item | Description |
|---|---|
| Several G13 | Several devices share one LCD pipe and the same configuration files. |
| Macro files | Changes to `macro-*.properties` only take effect the next time the bindings are loaded. |

### 4.2 Ideas

| Task | Description |
|---|---|
| Hotplug without polling | Device detection via libusb hotplug instead of polling every second |
| Joystick as analog axes | Offer the stick as a gamepad stick instead of four keys |
| LCD plugins | Clock, system load, "now playing" etc. via the FIFO |
| Profile switching from outside | e.g. via D-Bus from the GUI or other programs |
| Per-app profiles | Automatic profile switch depending on the active window |

---

## Phase 5 – Tests & quality assurance

| Task | Status |
|---|---|
| CI pipeline: build on Arch, Fedora and Debian testing with `-Werror` on every push and pull request | Done |
| Static analysis in CI: `clang-tidy` (`.clang-tidy`), `cppcheck`, `shellcheck`, `actionlint` | Done |
| Memory sanitizers via `ENABLE_SANITIZERS` (`-fsanitize=address,undefined`) | Available |
| Unit tests for the binding parser and the macro engine | Open |
| Driver tests against a simulated G13 (libusb/uinput mock) | Open |

---

## Phase 6 – Packaging & release

| Task | Status |
|---|---|
| Arch Linux – AUR package `linux-g13-driver-git`, published by the release workflow | Done |
| Version tags (`v1.0.0` Java version, `v2.0.0` Qt6) | Done |
| `make remove-legacy` to remove old installations | Done |
| `.deb` packages for Debian 12/13 and Ubuntu 22.04/24.04/26.04 (CPack) | Done |
| `.rpm` packages for Fedora 43/44 (CPack) | Done |
| GitHub release with changelog via GitHub Actions on tag push | Done |
| License file (`LICENSE`, GPL-2.0-only) and `AUTHORS` | Done |

---

## Technology stack (overview)

| Layer | Technology |
|---|---|
| Language | C++23 |
| Build | CMake 3.20+ (`GNUInstallDirs`), Makefile as a convenience wrapper |
| USB | libusb-1.0 |
| Keyboard events | uinput / `linux/input.h` |
| Device detection | Polling via libusb (1 s) |
| GUI | Qt6 (Widgets, Network for the single instance), Qt Linguist for translations |
| Service | systemd user unit, udev rule with `uaccess` |
