# Linux G13 Driver – Entwicklungs-Roadmap

Diese Roadmap beschreibt den geplanten Entwicklungsweg vom urspruenglichen Prototypen
bis hin zu einer stabilen, vollstaendig paketisierten Open-Source-Loesung.

---

## Phase 1 – Kern-Treiber (abgeschlossen)

**Ziel:** Funktionsfaehiger Daemon, der G13-Tasten per `uinput` an den Kernel weiterreicht.

| Aufgabe | Status |
|---|---|
| USB-Kommunikation via `libusb-1.0` | Fertig |
| Virtuelle Tastatur via `uinput` | Fertig |
| Binding-Dateien lesen (`.properties`) | Fertig |
| Joystick-Emulation | Fertig |
| LCD-Grundfunktionen (Buffer, Pixel, Text) | Fertig |
| Vier Binding-Profile (M1–M4) mit Live-Reload | Fertig |
| Makro-Unterstuetzung | Fertig |
| LCD-FIFO-Schnittstelle (`/tmp/g13-lcd`) | Fertig |
| `systemd`-Unit + `udev`-Regel | Fertig |

---

## Phase 2 – Refactoring & Architektur-Bereinigung (abgeschlossen)

**Ziel:** Saubere Trennung von Daemon und GUI, Modernisierung des Build-Systems.

| Aufgabe | Status |
|---|---|
| C++-Standard auf **C++23** angehoben | Fertig |
| CMake auf **3.20+** aktualisiert | Fertig |
| **GTK3 / AppIndicator vollstaendig entfernt** aus Daemon | Fertig |
| Daemon laeuft als reiner POSIX-Hintergrunddienst | Fertig |
| `BUILD_CONFIG_TOOL`, `BUILD_TESTS`, `ENABLE_SANITIZERS` Build-Optionen | Fertig |
| `libudev` als explizite Abhaengigkeit erkaert | Fertig |
| `GNUInstallDirs` fuer saubere Install-Pfade | Fertig |
| `CMAKE_EXPORT_COMPILE_COMMANDS` fuer Tooling (clang-tidy) | Fertig |
| Alle Globals in `Main.cpp` als `static` deklariert | Fertig |
| `std::map::contains` (C++20/23) statt `find() != end()` | Fertig |
| GTK-Callback-Signaturen (C-ABI) durch saubere C++-Signale ersetzt | Fertig |

---

## Phase 3 – Qt6 Config-Tool (naechster Schritt)

**Ziel:** Vollstaendige grafische Konfigurationsoberflaeche auf Basis von **Qt6** –
kein GTK, kein Java, keine externen Skripte.

### 3.1 Architektur

```
linux-g13-driver  (Daemon, kein GUI-Toolkit)
        |
        | IPC: Unix-Domain-Socket / D-Bus
        |
g13-gui           (Qt6-Anwendung)
   ├── MainWindow      – Hauptfenster
   ├── G13Visualizer   – G13-Tastaturansicht (klickbar)
   ├── BindingEditor   – Tastenbelegung pro Profil
   ├── ColorSelector   – Hintergrundbeleuchtung (RGB-Picker)
   └── MacroRecorder   – Makro aufzeichnen & abspielen
```

### 3.2 Geplante Aufgaben

| Aufgabe | Prioritaet |
|---|---|
| `QSystemTrayIcon` statt AppIndicator im GUI (nicht im Daemon) | Hoch |
| G13-Visualizer als `QWidget` mit SVG-Overlay | Hoch |
| Binding-Editor: Profil-Auswahl, Keycode-Mapping per Dropdown | Hoch |
| Farb-Picker via `QColorDialog` | Mittel |
| Makro-Recorder: Aufnahme per `libevdev` / `uinput` Replay | Mittel |
| Live-Vorschau: Aenderungen sofort an Daemon senden | Mittel |
| Qt6-Uebersetzungen (`Qt6LinguistTools`, `.ts`-Dateien) | Niedrig |

### 3.3 Abhaengigkeiten (GUI-only)

```cmake
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)
# Optional fuer D-Bus-IPC:
# find_package(Qt6 OPTIONAL_COMPONENTS DBus)
```

Der Daemon **benoetigt kein GUI-Toolkit** – er kommuniziert ueber IPC.

---

## Phase 4 – Erweiterte Funktionen

**Ziel:** Funktionsumfang auf Augenpunkt mit kommerziellen Logitech-Tools bringen.

| Aufgabe | Beschreibung |
|---|---|
| Hotplugging via `libudev` | Geraete ohne Neustart des Daemons erkennen |
| LCD-Plugins | Uhr, CPU-Auslastung, jetzt-spielend etc. ueber FIFO |
| Profilwechsel per D-Bus | GUI und andere Apps koennen Profile umschalten |
| Mehrere G13-Geraete | N gleichzeitige Geraete mit je eigenem Konfigurationsset |
| Per-App-Profile | Automatischer Profilwechsel anhand des aktiven Fensters |

---

## Phase 5 – Tests & Qualitaetssicherung

| Aufgabe | Werkzeug |
|---|---|
| Unit-Tests fuer Binding-Parser | GoogleTest |
| Unit-Tests fuer Makro-Engine | GoogleTest |
| Integrationstests (simuliertes USB-Geraet) | usbip + GoogleTest |
| Statische Analyse | `clang-tidy`, `cppcheck` |
| Speicher-Sanitizer | `-fsanitize=address,undefined` (via `ENABLE_SANITIZERS`) |
| CI-Pipeline | GitHub Actions: Build + Test auf Ubuntu 22.04 / 24.04 |

---

## Phase 6 – Paketierung & Release

| Aufgabe | Werkzeug |
|---|---|
| Debian/Ubuntu-Paket (`.deb`) | `CPack` + `dh_make` |
| Arch Linux – AUR-Paket (`PKGBUILD`) | manuell |
| RPM-Paket (Fedora/openSUSE) | `CPack` |
| GitHub Release mit Changelogs | `gh release create` |
| Dokumentation (Nutzerhandbuch) | Markdown + MkDocs |

---

## Technologie-Stack (Uebersicht)

| Schicht | Technologie | Begruendung |
|---|---|---|
| Sprache | **C++23** | `std::expected`, `ranges`, Coroutinen |
| Build | **CMake 3.20+** | `GNUInstallDirs`, `EXPORT_COMPILE_COMMANDS` |
| USB | **libusb-1.0** | Stabile, portable HID-Kommunikation |
| Kernel-Events | **uinput / linux/input.h** | Virtuelle Tastatur ohne Kernel-Modul |
| Hotplug | **libudev** | Geraeteerkennung ohne Polling |
| GUI | **Qt6** | Nativ, schnell, kein GTK-Toolkit-Overhead |
| Tests | **GoogleTest** | Standard im C++-Oekosystem |
