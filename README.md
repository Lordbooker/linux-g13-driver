# G13 Linux Driver & Config Tool (Modernized Fork)

A user-space driver and configuration tool for the **Logitech G13** gameboard on Linux.

The original project is over 10 years old. This fork modernizes it:

* **`linux-g13-driver`** – a C++23 background daemon. It talks to the G13 via `libusb`,
  passes key presses to the system through `uinput`, drives the LCD and the backlight.
  It runs as a systemd user service and needs **no GUI toolkit**.
* **`g13-gui`** – a new **Qt6** configuration tool with a tray icon that starts automatically
  at login. It replaces the former Java/Swing config tool and the GTK/AppIndicator tray icon of
  the daemon.

> Since version 2.0 the Qt6 config tool replaces the Java tool. Existing configuration files in
> `~/.config/g13/` keep the same format and remain usable.

## Installation from Packages (recommended)

Ready-made packages are attached to every [GitHub release](https://github.com/Lordbooker/linux-g13-driver/releases):

| System | Package |
|---|---|
| Debian 12 / 13 | `linux-g13-driver_<version>-1.debian12_amd64.deb` / `.debian13` |
| Ubuntu 22.04 / 24.04 / 26.04 and derivatives (Linux Mint, Pop!_OS, …) | `linux-g13-driver_<version>-1.ubuntu<release>_amd64.deb` |
| Fedora 43 / 44 and derivatives (Nobara, …) | `linux-g13-driver-<version>-1.fc43.x86_64.rpm` / `.fc44` |
| Arch Linux and derivatives | AUR: [`linux-g13-driver-git`](https://aur.archlinux.org/packages/linux-g13-driver-git) |

Pick the package matching your system (derivatives use the package of their base release,
e.g. Linux Mint 22 → Ubuntu 24.04), then:

```bash
# Debian / Ubuntu
sudo apt install ./linux-g13-driver_<version>-1.<system>_amd64.deb

# Fedora
sudo dnf install ./linux-g13-driver-<version>-1.fc<NN>.x86_64.rpm

# then, as your normal user:
systemctl --user enable --now g13
```

On immutable Fedora variants (e.g. Bazzite, Silverblue) the RPM has to be layered with
`rpm-ostree install`.

When upgrading from version 1.x, see
[Upgrading from the old Version](#upgrading-from-the-old-version-gtk-tray--java-config-tool) first.

The rest of this section describes building from source.

## Requirements

* C++ compiler with C++23 support, `make`, `pkg-config`
* CMake >= 3.20
* `libusb-1.0` (development headers)
* Qt6 Base (Widgets, Core, Gui, Network) – only needed for the config tool
* Qt6 Linguist tools (`lrelease`, CMake module `Qt6LinguistTools`) – translations of the config
  tool (`qt6-tools` on Arch, `qt6-tools-dev`, `qt6-tools-dev-tools` and `qt6-l10n-tools` on
  Debian/Ubuntu, `qt6-qttools-devel` on Fedora)
* Optional: Python 3 with `psutil` for the example LCD monitor script

GTK3, AppIndicator and Java are **no longer required**.

### Automated Dependency Installation

`install_deps.sh` detects the package manager (pacman, apt-get, dnf, zypper) and installs the
build dependencies:

```bash
cd g13-driver
./scripts/install_deps.sh
```

The package lists for Debian/Ubuntu and Fedora are tested; the Arch and openSUSE lists are untested.

## Build

```bash
cd g13-driver
make
```

`make` installs the dependencies (via `install_deps.sh`, requires sudo) and builds both the
daemon and the config tool into `g13-driver/build/`.

To build **without** touching system packages (e.g. for packaging):

```bash
make build-driver
```

Alternatively with CMake directly (use `build` as directory if you want to use `make install` afterwards):

```bash
cmake -S . -B build
cmake --build build
```

### Building packages yourself

`g13-driver/packaging/build-package.sh` builds a `.deb` or `.rpm` for the system it runs on
(intended for a fresh container of the target distribution), e.g.:

```bash
docker run --rm -v "$PWD:/src" -w /src debian:13 g13-driver/packaging/build-package.sh dist
```

Releases are built automatically by GitHub Actions (`.github/workflows/release.yml`): pushing
a tag such as `v2.1.0` builds the packages for all supported systems and publishes them as a
GitHub release with a list of changes. The workflow can also be started manually from the
*Actions* tab to build test packages without publishing a release.

CMake options:

| Option | Default | Meaning |
|---|---|---|
| `BUILD_CONFIG_TOOL` | `ON` | Build the Qt6 config tool `g13-gui` |
| `BUILD_TESTS` | `OFF` | Build tests (requires GoogleTest; no tests exist yet) |
| `ENABLE_SANITIZERS` | `OFF` | ASan/UBSan, only for `CMAKE_BUILD_TYPE=Debug` |

## Upgrading from the old Version (GTK tray / Java config tool)

Earlier versions installed a daemon with a GTK tray icon and a Java config tool, either
system-wide (`make install`) or into your home directory (`make install-user`).
Leftovers of the old version can keep the **old driver or the old config tool running**
after an upgrade – for example `~/.config/systemd/user/g13.service` takes precedence over the
new service, and `~/.local/bin/g13-gui` usually comes first in `PATH`.

Remove them **before** installing the new version (run without sudo; the script asks for sudo
itself when system files are affected):

```bash
cd g13-driver
make remove-legacy
```

The script lists everything it found and asks before removing anything. It

* identifies old files by their content (Java wrapper, the old daemon linked against GTK/AppIndicator,
  the generated user service), so it works on every distribution,
* never touches files owned by a package manager (pacman, dpkg, rpm) – those are only listed and
  have to be removed with the package manager,
* stops the old service if the old driver is affected,
* keeps your configuration in `~/.config/g13`.

`sudo make install` also checks for leftovers and prints a warning, but does not remove anything.

| Old file | Origin |
|---|---|
| `~/.local/bin/linux-g13-driver`, `g13-gui`, `Linux-G13-GUI.jar` | `make install-user` |
| `~/.config/systemd/user/g13.service` | `make install-user` |
| `/etc/udev/rules.d/99-g13.rules` | `make install-user` |
| `/usr/share/java/linux-g13-driver/Linux-G13-GUI.jar` | `make install` |
| `/usr/bin/g13-gui` (Java wrapper), `/usr/bin/linux-g13-driver` (GTK version) | `make install` |

The same paths below `/usr/local` are checked as well.

## Installation

```bash
sudo make install
```

This installs:

| File | Target |
|---|---|
| Daemon | `/usr/bin/linux-g13-driver` |
| Config tool | `/usr/bin/g13-gui` |
| udev rule | `/usr/lib/udev/rules.d/99-g13.rules` |
| systemd user service | `/usr/lib/systemd/user/g13.service` |
| Application menu entry | `/usr/share/applications/g13-gui.desktop` |
| Tray icon autostart | `/etc/xdg/autostart/g13-gui-tray.desktop` |
| Icons | `/usr/share/icons/hicolor/*/apps/g13-gui.{png,svg}` |

`PREFIX` and `DESTDIR` can be overridden for packaging, e.g. `make install DESTDIR=pkgdir`.

The udev rule gives the logged-in user access to the G13 and to `uinput` without root.
Reload the rules once (or unplug and replug the G13):

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

The service is not started automatically. Enable it once for your user:

```bash
systemctl --user enable --now g13
```

A user-local installation (`make install-user`) is not supported in this version.

## Controlling the Driver

```bash
systemctl --user status g13      # status
journalctl --user -u g13 -f      # live log
systemctl --user restart g13     # restart
```

## Tray Icon

After the next login, the **G13 tray icon** appears in the taskbar (started via XDG autostart
with `g13-gui --tray`). To show it right away without logging out:

```bash
g13-gui --tray &
```

* **Left click** – open or hide the configuration window.
* **Right click** – menu:
  * *Open configuration*
  * driver status (*running* / *stopped*), *Start/Stop driver*, *Restart driver*
    (controls the systemd user service `g13`)
  * *Quit tray icon* – closes only the tray icon; the driver keeps running.

The driver itself runs independently of the tray icon as a systemd user service.

**GNOME:** tray icons are only shown with the extension
*AppIndicator and KStatusNotifierItem Support* (same as with the former AppIndicator icon).
Without a tray, `g13-gui --tray` exits quietly after 30 seconds; start the config tool from the
application menu instead.

**Disable the autostart:** turn off "G13 Tray Icon" (shown translated on non-English desktops,
e.g. "G13 Tray-Symbol") in your desktop's autostart settings, or
create `~/.config/autostart/g13-gui-tray.desktop` containing:

```ini
[Desktop Entry]
Hidden=true
```

## Using the Config Tool

Start it from the application menu (**G13 Config Tool**), the tray icon, or a terminal:

```bash
g13-gui
```

Only one instance runs at a time: starting `g13-gui` again opens the window of the running
instance.

Select the profile to edit in the toolbar (**Profile 1–4**, selected on the device with
**L1–L4**, correspond to `bindings-0.properties` … `bindings-3.properties`; the active profile's
L key is highlighted in the image). All changes are written directly to `~/.config/g13/`.

* **Key Bindings** – click a key in the G13 image (keys with a green dot are bound in the
  active profile; dashed outlines mark the profile keys L1–L4, which cannot be bound), then
  choose:
  * *Key code (pass-through)*: the key sends a normal keyboard key. Click into the input field
    and press the key on your keyboard – it is detected directly, including Tab, Enter, Esc,
    function keys and single modifiers (Shift, Ctrl, Alt, Super). One key per G13 key; for
    combinations such as Ctrl+C use a macro. Only keys your keyboard has can be assigned.
  * *Macro*: the key runs a macro – choose the macro ID and the *Run mode*: *Run once*,
    *Repeat while held* or *Run several times* (with a count),
  * *No action*: the key is unbound.

  Then click *Save*.
* **Backlight Color** – choose the RGB color of the profile.
* **Macro Recorder** – create macros:
  * *Start recording*: records key presses including their timing. The window must be in the
    foreground; shortcuts handled by your desktop (e.g. Alt+Tab) cannot be recorded.
  * The macro is shown as a list of steps (press, release, wait). Steps can be moved, removed
    or inserted manually: press a key in the *Key* field, then *Insert press* or
    *Insert release*.
  * *Apply to all* replaces all recorded delays with a fixed value.
  * *Save* warns if a key is pressed but never released.

Keys are shown with the label of your own keyboard layout (e.g. *Z* on a German keyboard)
once they have been pressed in the tool; keys never pressed there are shown with their US
layout name. The labels are stored in `~/.config/linux-g13-project/G13 Config Tool.conf`.

Closing the window keeps the tool running in the system tray (if your desktop provides one).
*File → Quit* or *Quit tray icon* closes it completely; the driver keeps running.

### Languages

The config tool follows the system language (`LANGUAGE`, `LC_ALL`, `LC_MESSAGES`, `LANG`).
Translations are included for **German, French, Italian, Spanish and Portuguese**; all other
languages use English. The translations are built into `g13-gui`, no extra files are installed.
Standard dialog buttons (e.g. *Cancel*) come from Qt's own translations – package
`qt6-translations` (Arch), `qt6-translations-l10n` (Debian/Ubuntu) or `qt6-qttranslations`
(Fedora). To start the tool in another language, e.g. English:

```bash
LANGUAGE=en g13-gui
```

The driver's log messages (`journalctl --user -u g13`) are always English.

**Adding a language:** add the language code to `G13_GUI_LANGUAGES` in
`g13-driver/CMakeLists.txt`, run `cmake --build build --target update_translations` to create
`g13-driver/translations/g13-gui_<code>.ts`, translate it with Qt Linguist (`linguist6` or
`linguist`) and rebuild. The same target updates all `.ts` files after text changes in the code.

### When do changes take effect?

The daemon watches the bindings file of the **active** profile and reloads it automatically
when it changes. Macro files are read whenever the bindings are (re)loaded – a changed macro
becomes active after the next change of the profile's bindings file, a profile switch, or a
restart of the driver.

## Profiles on the Device

The four buttons directly below the display (**L1–L4**) switch between `bindings-0` …
`bindings-3`. These buttons cannot be bound to other actions.

The buttons **M1, M2, M3 and MR** are freely bindable like the G keys.
The button with the sun symbol controls the display brightness in hardware and cannot be bound.

## Configuration Files (manual editing)

All files are located in `~/.config/g13/` (or `$XDG_CONFIG_HOME/g13/`).
If a bindings file is missing, the driver creates it with a default mapping on first use.

### `bindings-<0..3>.properties`

```ini
# backlight R,G,B (0-255)
color=0,0,255
# pass-through: physical key G20 sends Linux key code 20 (T)
G19=p,k.20
# physical key G5 runs macro 3 once (repeat mode 0)
G4=m,3,0
```

Comments must be on their own line starting with `#`. A comment at the end of a line is read
as part of the value and makes the line invalid.

Keys without an entry have **no function** in that profile (they do not keep the binding of
the previously active profile).

**Key numbering in the file is zero-based**: the file key `G0` is the physical key **G1**,
`G21` is **G22**. Further keys (bit positions as defined in `Constants.h`):

| File key | Physical key |
|---|---|
| `G0` … `G21` | G1 … G22 |
| `G24` | BD (round button left below the display) |
| `G29`, `G30`, `G31`, `G32` | M1, M2, M3, MR |
| `G33`, `G34`, `G35` | thumb keys LEFT, DOWN, TOP (stick press) |
| `G36`, `G37`, `G38`, `G39` | joystick up, left, right, down |

The joystick directions can be set in the GUI (click the ring around the stick) or in
the file (default: W, A, D, S).

Key codes are Linux input event codes, see `docs/Eventcodes_for_Mapping.pdf` or
`/usr/include/linux/input-event-codes.h` (e.g. `KEY_T` = 20).

**Macro repeat mode** (`m,<macroId>,<repeats>`):

| Value | Behaviour |
|---|---|
| `0` | run once |
| `1` | repeat while the key is held |
| `>1` | run the given number of times |

Pressing the key again while a macro is running stops it.

### `macro-<0..199>.properties`

```ini
name=Copy
id=3
sequence=kd.29,d.40,kd.46,d.60,ku.46,d.30,ku.29
```

| Token | Meaning |
|---|---|
| `kd.<code>` | press key |
| `ku.<code>` | release key |
| `d.<ms>` | wait (milliseconds) |

Tokens are separated by commas without spaces.

## Use with External Tools

The driver works without the config tool: it creates a default mapping if no configuration
exists. You can then remap the keys with tools like **Input Remapper**.

## LCD Scripting

The driver creates a named pipe (FIFO); text written to it is shown on the LCD:

* `$XDG_RUNTIME_DIR/g13-lcd` – usually `/run/user/<uid>/g13-lcd`
* `/tmp/g13-lcd` – only if `XDG_RUNTIME_DIR` is not set

```bash
PIPE="$XDG_RUNTIME_DIR/g13-lcd"

echo "Hello World!" > "$PIPE"
echo -e "CPU: 50%\nRAM: 4GB" > "$PIPE"
```

Only one font (5×7) is implemented. An example monitor script is located in
`g13-driver/scripts/g13_monitor.py`.

> **Known limitation:** `g13_monitor.py` currently writes to `/tmp/g13-lcd` only and therefore
> does not find the pipe when `XDG_RUNTIME_DIR` is set (the normal case).

## Uninstallation

```bash
systemctl --user disable --now g13
cd g13-driver
sudo make uninstall
```

This removes the daemon, the config tool, the udev rule, the service file, the menu entry,
the tray autostart entry and the icons.
Your configuration in `~/.config/g13` is kept.

## Notes

* Developed on 64-bit Arch Linux.
* Planned next steps are described in [ROADMAP.md](ROADMAP.md).

## License and Credits

This project is licensed under the **GNU General Public License, version 2 only**
(GPL-2.0-only) – see [LICENSE](LICENSE).

It is a fork of [linux-g13-driver](https://code.google.com/archive/p/linux-g13-driver) by
**Jim Gupta** (2011), which was published under the GNU GPL v2. According to its project
description, parts of the original driver code were based on
[ecraven/g13](https://github.com/ecraven/g13). Further contributors are listed in
[AUTHORS](AUTHORS) and in the git history.
