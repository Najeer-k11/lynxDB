# lynxDB — Terminal Database Viewer

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](CMakeLists.txt)

**lynxDB** is a fast, information-dense, keyboard-driven Linux terminal database explorer built with **C++20**, **CMake**, **ncursesw**, **MySQL/MariaDB (`libmariadb`)**, and **SQLite3 (`sqlite3`)**.

- **Author**: Venkata Najeer Kopparapu
- **GitHub**: [github.com/Najeer-k11](https://github.com/Najeer-k11)
- **License**: Free and Open Source under the MIT License

---

## Key Features

- **Multi-Database Support**: Native connection drivers for **MySQL / MariaDB** and **SQLite3**.
- **Interactive Three-Panel TUI**:
  - **Left Navigation Sidebar**: Tree explorer showing Server → Databases → Tables / Views with smooth vertical scrolling and expand/collapse hierarchy.
  - **Right Main Content Area**: Dynamic table data viewer & schema structure viewer with horizontal column scrolling, NULL representation, VS Code-style vertical line number gutter, and row index range header (`Rows 1–25 of 150`).
  - **Bottom Status Line**: Real-time shortcut list, horizontal column position indicator (`Cols 1–4 of 12`), active theme name, and status notifications.
- **Foreign Key Navigation (`f`)**: Highlight any foreign key cell (e.g., `user_id = 5`) and press `f` to jump directly to referenced table (`users`) at row `id = 5` (with heuristic fallback).
- **TUI ASCII ER Diagram Viewer (`V`)**: Interactive ASCII ER relationship diagram visualizing database tables, column types, `[PK]` / `[FK]` badges, and relationships.
- **Go to Line / Row (`g` / `G`)**: Jump to any row number directly via a quick input prompt modal.
- **6 Neon Accent Themes**: Live cycling via `a` or `F2` key (Neon Cyan, Neon Green, Neon Magenta, Neon Yellow, Neon Red, Neon Blue) with high-contrast text rendering and local preference persistence (`~/.config/lynxdb/theme.cfg`).
- **Data Export (`E`)**: Export active table rows or query result sets directly to **CSV** or **JSON** files with full path resolution.
- **Full Data CRUD**:
  - **Single-Cell Edit (`e`)**: Inline cell editing with primary key validation and pre-execution `UPDATE` confirmation overlay (`ConfirmDialog`).
  - **Row Delete (`d`)**: Delete selected row with primary key validation and pre-execution `DELETE` confirmation overlay.
  - **Row Insert (`i`)**: Insert new row using a structured multi-field form modal (`RowInsertDialog`).
- **In-UI Table Row Filter (`/`)**: Real-time search/filter across all columns with live matching row count.
- **Column Sorting (`o`)**: Toggle Ascending/Descending sort on selected column with `▲`/`▼` header indicator.
- **Schema DDL Management (`N`)**: Interactive modal to Create/Drop Tables and Views, Add/Rename/Drop Columns, Create/Drop Indexes, and set Constraints.
- **Clipboard Copying (`y` / `Y`)**: Copy cell value (`y`) or full tab-separated row (`Y`) using OSC 52 terminal escape sequences (`\033]52;c;<base64>\007`) with internal buffer fallback. `NULL` cells copy as empty strings (`""`).
- **Encrypted Credential Storage**: Auto-encrypts saved database passwords in `~/.config/lynxdb/connections.cfg`.

---

## Keyboard Controls

| Key | Action |
| --- | --- |
| `c` / `n` | Open Connection Dialog (Localhost root/admin, Remote MySQL, SQLite, URI) |
| `f` | Jump to referenced row via Foreign Key relationship |
| `V` | Open TUI ASCII ER Relationship Diagram Viewer |
| `g` / `G` | Jump to Row line number prompt |
| `:` | Open multiline SQL Query execution prompt (`Enter` newline, `F5` run) |
| `/` | Open in-UI search filter prompt |
| `E` | Export active table data or query results to CSV or JSON |
| `i` | Insert new row into active table |
| `d` | Delete selected row (requires primary key validation) |
| `e` / `E` | Edit selected cell (requires primary key, confirms `UPDATE` query) |
| `o` | Toggle Ascending/Descending sort on selected column |
| `N` | Open Schema DDL Management modal (Create/Drop Table, View, Column, Index) |
| `a` / `F2` | Cycle through 6 Neon Accent Color Themes |
| `y` | Copy selected cell value via OSC 52 (copies `""` for `NULL`) |
| `Y` | Copy full selected row (tab-separated) via OSC 52 |
| `s` / `S` | View Table Structure (`DESCRIBE <table>` / `PRAGMA table_info`) |
| `t` / `T` | View Table Data (`SELECT * FROM <table>`) |
| `Tab` | Switch active panel focus (Sidebar ↔ Content Area) |
| `←` / `→` / `h` / `l` | Scroll columns horizontally in Content Table / Collapse & Expand tree in Sidebar |
| `↑` / `↓` / `j` / `k` | Navigate rows up/down in active panel |
| `PageUp` / `PageDown` | Page table data / sidebar list up/down |
| `r` / `R` | Refresh database metadata or table view |
| `x` / `Del` | Remove highlighted saved connection on Welcome Screen |
| `q` / `Q` / `Ctrl+C` | Quit application cleanly |

---

## Native Packaging Suite for All Linux Distributions

```text
packaging/
├── debian/              (Debian, Ubuntu, Linux Mint, Pop!_OS)
├── lynxdb.spec          (Fedora, RHEL, CentOS, Nobara, openSUSE)
└── PKGBUILD             (Arch Linux, Manjaro, EndeavourOS)
```

### Quick Install Commands
- **Fedora / Nobara / RHEL**: `sudo dnf install ./lynxdb-0.1.0-Linux.rpm`
- **Debian / Ubuntu**: `sudo dpkg -i lynxdb-0.1.0-Linux.deb`
- **Arch Linux / AUR**: `cd packaging && makepkg -si`

---

## License

Free and Open Source under the **MIT License**. Created by [Venkata Najeer Kopparapu](https://github.com/Najeer-k11).
