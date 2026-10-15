# G13 Linux Driver & Config Tool (Modernized Fork)

A user-space driver and configuration tool for the **Logitech G13** gameboard on Linux.

The original project is over 10 years old. This fork modernizes it:

* **`linux-g13-driver`** – a C++23 background daemon. It talks to the G13 via `libusb`,
  passes key presses to the system through `uinput`, drives the LCD and the backlight.
  It runs as a systemd user service and needs **no GUI toolkit**.
* **`g13-gui`** – a new **Qt6** configuration tool with its own tray icon.
  It replaces the former Java/Swing config tool and the GTK/AppIndicator tray icon of the daemon.

> **Status:** The Qt6 config tool is under active development on the `refactoring` branch.
> Existing configuration files in `~/.config/g13/` keep the same format and remain usable.

## Requirements

* C++ compiler with C++23 support, `make`, `pkg-config`
* CMake >= 3.20
* `libusb-1.0` (development headers)
* Qt6 Base (Widgets, Core, Gui) – only needed for the config tool
* Optional: Python 3 with `psutil` for the example LCD monitor script

GTK3, AppIndicator and Java are **no longer required**.

### Automated Dependency Installation

`install_deps.sh` detects the package manager (pacman, apt-get, dnf, zypper) and installs the
build dependencies:

```bash
cd g13-driver
./scripts/install_deps.sh
```

The package lists for Fedora and openSUSE have not been tested yet.

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

## Using the Config Tool

Start it from your application menu or a terminal:

```bash
g13-gui
```

Select the profile to edit in the toolbar (**Profil 1–4**, selected on the device with
**L1–L4**, correspond to `bindings-0.properties` … `bindings-3.properties`; the active profile's
L key is highlighted in the image). All changes are written directly to `~/.config/g13/`.

* **Key Bindings** – click a key in the G13 image (keys with a green dot are bound in the
  active profile; dashed outlines mark profile/LCD keys that cannot be bound), then choose:
  * *Tastencode (Pass-Through)*: the key sends a normal keyboard key,
  * *Makro*: the key runs a macro (macro ID and repeat mode),
  * *Keine Aktion*: the key is unbound.

  Then click *Speichern*.
* **Backlight Color** – choose the RGB color of the profile.
* **Macro Recorder** – create macros:
  * *Aufnahme starten*: records key presses including their timing. The window must be in the
    foreground; shortcuts handled by your desktop (e.g. Alt+Tab) cannot be recorded.
  * The macro is shown as a list of steps (press, release, wait). Steps can be moved, removed
    or inserted manually.
  * *Auf alle anwenden* replaces all recorded delays with a fixed value.
  * *Speichern* warns if a key is pressed but never released.

Closing the window keeps the tool running in the system tray (if your desktop provides one);
quit it via the tray menu (*Beenden*).

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
color=0,0,255        # backlight R,G,B (0-255)
G19=p,k.20           # pass-through: send Linux key code 20 (T)
G4=m,3,0             # macro 3, repeat mode 0
```

Keys without an entry have **no function** in that profile (they do not keep the binding of
the previously active profile).

**Key numbering in the file is zero-based**: the file key `G0` is the physical key **G1**,
`G21` is **G22**. Further keys (bit positions as defined in `Constants.h`):

| File key | Physical key |
|---|---|
| `G0` … `G21` | G1 … G22 |
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

This removes the daemon, the config tool, the udev rule and the service file.
Your configuration in `~/.config/g13` is kept.

## Notes

* Developed on 64-bit Arch Linux.
* Planned next steps are described in [ROADMAP.md](ROADMAP.md).
