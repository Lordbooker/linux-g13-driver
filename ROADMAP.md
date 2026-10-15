# Linux G13 Driver – Entwicklungs-Roadmap

Diese Roadmap beschreibt den Entwicklungsweg vom ursprünglichen Prototypen bis zu einer stabilen,
paketierten Open-Source-Lösung. Stand: Version 2.0.

---

## Phase 1 – Kern-Treiber (abgeschlossen)

**Ziel:** Funktionsfähiger Daemon, der G13-Tasten per `uinput` an das System weiterreicht.

| Aufgabe | Status |
|---|---|
| USB-Kommunikation via `libusb-1.0` | Fertig |
| Virtuelle Tastatur via `uinput` | Fertig |
| Binding-Dateien lesen (`.properties`) | Fertig |
| Joystick-Emulation (Tasten-Modus) | Fertig |
| LCD-Grundfunktionen (Buffer, Pixel, Text) | Fertig |
| Vier Binding-Profile (Umschaltung per L1–L4) mit Live-Reload | Fertig |
| Makro-Unterstützung | Fertig |
| LCD-FIFO-Schnittstelle (`$XDG_RUNTIME_DIR/g13-lcd`, Fallback `/tmp/g13-lcd`) | Fertig |
| `systemd`-User-Unit + `udev`-Regel | Fertig |

---

## Phase 2 – Refactoring & Robustheit (abgeschlossen)

**Ziel:** Daemon ohne GUI-Toolkit, modernes Build-System, stabiles Verhalten im Fehlerfall.

| Aufgabe | Status |
|---|---|
| C++-Standard auf **C++23**, CMake **3.20+** | Fertig |
| **GTK3 / AppIndicator aus dem Daemon entfernt** – reiner Hintergrunddienst | Fertig |
| Build-Optionen `BUILD_CONFIG_TOOL`, `BUILD_TESTS`, `ENABLE_SANITIZERS`; `GNUInstallDirs` | Fertig |
| Warnungsfreier Build (`-Wall -Wextra`) | Fertig |
| Sauberes Herunterfahren: Signal-Handler setzt nur ein Flag, alle Geräte-Threads werden gejoint | Fertig |
| Belegungen werden vor jedem Laden zurückgesetzt (nicht eingetragene Tasten ohne Funktion) | Fertig |
| Keine hängenden Tasten bei Profilwechsel/Live-Reload; Profilwechsel nur bei Tastendruck | Fertig |
| Neuversuch nach fehlgeschlagenem Öffnen; beendete Geräte-Threads werden aufgeräumt | Fertig |
| USB-Lesefehler: Pause, gedrosseltes Logging, Freigabe und Neuöffnen nach Fehlerserie | Fertig |
| Ungültige `color`-Werte, Init-Fehler und LCD-Zeichen 127 führen nicht mehr zu Absturz/UB | Fertig |

---

## Phase 3 – Qt6 Config-Tool (abgeschlossen, v2.0)

**Ziel:** Grafische Konfiguration auf Basis von **Qt6** als Ersatz für das Java-Tool.

### 3.1 Architektur

```
linux-g13-driver  (systemd-User-Dienst, kein GUI-Toolkit)
        ^
        |  Konfigurationsdateien ~/.config/g13/*.properties
        |  (Treiber: Live-Reload der Bindings-Datei des aktiven Profils)
        |
g13-gui           (Qt6-Anwendung, Tray-Symbol per XDG-Autostart)
   ├── MainWindow      – Hauptfenster, Tray-Menü, Einzelinstanz
   ├── G13Visualizer   – klickbares G13-Bild
   ├── BindingEditor   – Tastenbelegung pro Profil
   ├── ColorSelector   – Hintergrundbeleuchtung (RGB)
   └── MacroRecorder   – Makros aufnehmen und bearbeiten
```

GUI und Treiber kommunizieren ausschließlich über die Konfigurationsdateien; eine direkte
Schnittstelle (IPC) gibt es nicht. Das Tray-Menü steuert den Treiber über `systemctl --user`.

### 3.2 Umgesetzt

| Aufgabe | Status |
|---|---|
| Klickbares G13-Bild (unverzerrt, skalierend) inkl. M1–M3/MR, BD, Daumentasten, Joystick-Richtungen | Fertig |
| Markierung belegter Tasten und des aktiven Profils (L1–L4) | Fertig |
| Binding-Editor: Tastencode, Makro (einmal / solange gedrückt / n-mal), keine Aktion | Fertig |
| Farbwahl pro Profil | Fertig |
| Makro-Editor: Aufnahme mit Zeitabständen, Ereignisliste, manuelles Einfügen, feste Verzögerung | Fertig |
| Tray-Symbol: Konfiguration öffnen, Treiber starten/stoppen/neu starten, Status | Fertig |
| XDG-Autostart des Tray-Symbols, Menüeintrag, eigenes App-Icon | Fertig |
| Einzelinstanz: erneuter Start öffnet das vorhandene Fenster | Fertig |
| Mehrsprachigkeit nach Systemsprache (Qt Linguist): Englisch als Quelle/Rückfall, Deutsch, Französisch, Italienisch, Spanisch, Portugiesisch | Fertig |
| Treiber-Logmeldungen einheitlich Englisch | Fertig |

---

## Phase 4 – Offene Punkte und erweiterte Funktionen

### 4.1 Bekannte Einschränkungen

| Punkt | Beschreibung |
|---|---|
| Makros blockieren beim Stoppen | Wird ein laufendes Makro gestoppt oder ersetzt, wartet der Lese-Thread bis zum Ende des aktuellen Warteschritts (`d.<ms>`). |
| Zugriffsrechte | udev-Regel setzt zusätzlich zu `uaccess` `MODE="0666"`; die LCD-Pipe wird mit 0666 angelegt (kritisch nur im `/tmp`-Fallback). |
| Mehrere G13 | Mehrere Geräte teilen sich eine LCD-Pipe und dieselben Konfigurationsdateien. |
| Default-Bindings | Die eingebauten Standardbelegungen enthalten `G22`/`G23`, die keiner Taste entsprechen. |
| Makro-Dateien | Änderungen an `macro-*.properties` wirken erst beim nächsten Laden der Bindings. |
| `g13_monitor.py` | Schreibt nur nach `/tmp/g13-lcd` und findet die Pipe unter `$XDG_RUNTIME_DIR` nicht. |
| Kleinigkeiten | Veralteter `install-user`-Kommentar in `g13.service`. |

### 4.2 Ideen

| Aufgabe | Beschreibung |
|---|---|
| Hotplug ohne Polling | Geräteerkennung per libusb-Hotplug statt 1-Sekunden-Abfrage |
| Joystick-Absolutmodus | Im Treiber vorhanden, aber nicht konfigurierbar |
| LCD-Plugins | Uhr, Systemlast, „jetzt läuft“ usw. über die FIFO |
| Profilwechsel von außen | z. B. per D-Bus durch GUI oder andere Programme |
| Per-App-Profile | Automatischer Profilwechsel je nach aktivem Fenster |

---

## Phase 5 – Tests & Qualitätssicherung (offen)

Im Repository gibt es bisher keine automatisierten Tests.

| Aufgabe | Werkzeug |
|---|---|
| Unit-Tests für Binding-Parser und Makro-Engine | GoogleTest |
| Treibertests gegen ein simuliertes G13 (libusb-/uinput-Attrappe) | GoogleTest |
| Statische Analyse | `clang-tidy`, `cppcheck` |
| Speicher-Sanitizer | `-fsanitize=address,undefined` (via `ENABLE_SANITIZERS`) |
| CI-Pipeline | GitHub Actions: Build und Tests bei jedem Push |

---

## Phase 6 – Paketierung & Release

| Aufgabe | Status |
|---|---|
| Arch Linux – AUR-Paket `linux-g13-driver-git` | Fertig |
| Versions-Tags (`v1.0.0` Java-Stand, `v2.0.0` Qt6) | Fertig |
| `make remove-legacy` zum Entfernen alter Installationen | Fertig |
| `.deb`-Pakete für Debian 12/13 und Ubuntu 22.04/24.04/26.04 (CPack) | Fertig |
| `.rpm`-Pakete für Fedora 43/44 (CPack) | Fertig |
| GitHub-Release mit Changelog per GitHub Actions bei Tag-Push | Fertig |
| Lizenzdatei (`LICENSE`, GPL-2.0-only) und `AUTHORS` | Fertig |

---

## Technologie-Stack (Übersicht)

| Schicht | Technologie |
|---|---|
| Sprache | C++23 |
| Build | CMake 3.20+ (`GNUInstallDirs`), Makefile als Komfort-Wrapper |
| USB | libusb-1.0 |
| Tastatur-Ereignisse | uinput / `linux/input.h` |
| Geräteerkennung | Polling über libusb (1 s) |
| GUI | Qt6 (Widgets, Network für die Einzelinstanz), Qt Linguist für Übersetzungen |
| Dienst | systemd-User-Unit, udev-Regel mit `uaccess` |
