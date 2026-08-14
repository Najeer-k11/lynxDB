# lynxDB — Terminal Database Viewer

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](CMakeLists.txt)

**lynxDB** is a fast, information-dense, keyboard-driven Linux terminal database explorer built with **C++20**, **CMake**, **ncursesw**, **MySQL/MariaDB (`libmariadb`)**, and **SQLite3 (`sqlite3`)**.

- **Author**: Venkata Najeer Kopparapu
- **GitHub**: [github.com/Najeer-k11](https://github.com/Najeer-k11)
- **License**: Free and Open Source under the MIT License

---

## Preview

![Welcome Screen & Saved Connections](https://raw.githubusercontent.com/Najeer-k11/lynxDB/main/preview/1.png)

![Database Explorer & Content Table](https://raw.githubusercontent.com/Najeer-k11/lynxDB/main/preview/2.png)

![Interactive Table Row Filtering](https://raw.githubusercontent.com/Najeer-k11/lynxDB/main/preview/3.png)

![Dynamic Accent Themes](https://raw.githubusercontent.com/Najeer-k11/lynxDB/main/preview/4.png)

---

## Features

- **Multi-Database Support**: Native connection drivers for **MySQL / MariaDB** and **SQLite3**.
- **Interactive Three-Panel TUI**:
  - **Left Navigation Sidebar**: Tree explorer showing Server → Databases → Tables / Views with expand/collapse hierarchy.
  - **Right Main Content Area**: Dynamic table data viewer & schema structure viewer with horizontal column scrolling, NULL representation, and vertical row scrolling.
  - **Bottom Status Line**: Real-time shortcut list, horizontal column position indicator (`Cols 1–4 of 12`), and status notifications.
- **Interactive Modal Dialogs, Filtering & Editing**:
  - **Connection Dialog (`c` / `n`)**: Edit connection parameters (Host, Port, User, Password, Database, DB Type) or paste connection URIs (`mysql://...` or `sqlite://...`). Presets for Localhost root/admin and Online remote databases.
  - **Table Row Filtering (`/`)**: Instant row filter modal in table view to filter data rows across all columns in real-time.
  - **SQL Query Console (`:`)**: Multiline SQL statement buffer (`Enter` for newline) executed via `F5`.
  - **Single-Cell Edit (`e`)**: Inline cell editing with primary key validation and pre-execution `UPDATE` confirmation overlay (`ConfirmDialog`).
  - **Clipboard Copying (`y` / `Y`)**: Copy cell value (`y`) or full tab-separated row (`Y`) using OSC 52 terminal escape sequences (`\033]52;c;<base64>\007`) with internal buffer fallback. `NULL` cells copy as empty strings (`""`).
  - **Error Popup Overlay**: Displays verbatim database connection/query errors inside the UI without crashing.
- **Dynamic Accent Themes (`a` / `F2`)**: Cycle vibrant neon accent themes (Cyan, Green, Magenta, Yellow, Red, Blue) on the fly with live UI refresh.
- **Persistence**: Auto-saves connection configurations (`~/.config/lynxdb/connections.cfg`) and active accent theme (`~/.config/lynxdb/theme.cfg`).

---

## Native Packaging Suite for All Linux Distributions

`lynxDB` includes native packaging definitions for all major Linux distribution families under `packaging/`:

```text
packaging/
├── debian/              (Debian, Ubuntu, Linux Mint, Pop!_OS)
│   ├── control
│   ├── rules
│   └── changelog
├── lynxdb.spec          (Fedora, RHEL, CentOS, Nobara, openSUSE)
└── PKGBUILD             (Arch Linux, Manjaro, EndeavourOS)
```

---

### 1. Build & Install Native Packages

#### A. Debian / Ubuntu / Linux Mint (`.deb`)
```bash
# Option 1: Native debhelper build
cd packaging
dpkg-buildpackage -us -uc -b

# Option 2: CMake CPack generator
cmake -B build -S .
cd build && cpack -G DEB
sudo dpkg -i lynxdb-0.1.0-Linux.deb
```

#### B. Fedora / RHEL / CentOS / Nobara (`.rpm`)
```bash
# Option 1: Native rpmbuild
rpmbuild -ba packaging/lynxdb.spec

# Option 2: CMake CPack generator
cmake -B build -S .
cd build && cpack -G RPM
sudo dnf install ./lynxdb-0.1.0-Linux.rpm
```

#### C. Arch Linux / Manjaro / EndeavourOS (`PKGBUILD`)
```bash
cd packaging
makepkg -si
```

#### D. Universal Linux Tarball (`.tar.gz`)
```bash
cmake -B build -S .
cd build && cpack -G TGZ
```

---

## Building from Source

### Prerequisites
- **Fedora/RHEL**: `sudo dnf install -y cmake gcc-c++ ncurses-devel mariadb-connector-c-devel sqlite-devel`
- **Ubuntu/Debian**: `sudo apt install -y build-essential cmake libncursesw5-dev libmariadb-dev libsqlite3-dev`
- **Arch Linux**: `sudo pacman -S --needed base-devel cmake ncurses mariadb-libs sqlite`

### Build Commands
```bash
mkdir -p build
cmake -B build -S .
cmake --build build
./build/lynxdb
```

---

## Keyboard Controls

| Key | Action |
| --- | --- |
| `c` / `n` | Open Connection Dialog (Localhost root/admin, Remote MySQL, SQLite, URI) |
| `/` | Open interactive table row filter prompt (in Table View) |
| `a` / `F2` | Cycle neon accent color themes (Cyan, Green, Magenta, Yellow, Red, Blue) |
| `:` | Open multiline SQL Query execution prompt |
| `F5` / `Ctrl+R` | Execute query inside SQL Query prompt |
| `e` / `E` | Edit selected cell (requires primary key, confirms `UPDATE` query) |
| `y` | Copy selected cell value via OSC 52 (copies `""` for `NULL`) |
| `Y` | Copy full selected row (tab-separated) via OSC 52 |
| `s` / `S` | View Table Structure (`DESCRIBE <table>` / `PRAGMA table_info`) |
| `t` / `T` | View Table Data (`SELECT * FROM <table>`) |
| `Tab` | Switch active panel focus (Sidebar ↔ Content Area) |
| `←` / `→` / `h` / `l` | Scroll columns horizontally in Content Table / Collapse & Expand tree in Sidebar |
| `↑` / `↓` / `j` / `k` | Navigate rows up/down in active panel |
| `PageUp` / `PageDown` | Page table data view up/down |
| `r` / `R` | Refresh database metadata or table view |
| `q` / `Q` / `Ctrl+C` | Quit application cleanly |

---

## License

Free and Open Source under the **MIT License**. Created by [Venkata Najeer Kopparapu](https://github.com/Najeer-k11).
