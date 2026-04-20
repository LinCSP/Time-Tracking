# ⏱ Time Tracker

A minimalist work time tracking app built with Qt6 and a dark theme.

[Русский](README.ru.md)

## Features

- **Projects** — create projects with color-coded labels
- **Auto-switch** — starting a new project automatically stops the current timer
- **Session history** — full log of all work sessions with per-project filtering
- **Persistence** — data stored in SQLite; active session is restored on restart
- **Multilingual** — English / Russian, language preference saved between sessions
- **Custom window** — frameless window with draggable title bar; double-click to maximize

## Screenshots

| Projects | History |
|----------|---------|
| Dark theme with color-coded project cards | Session table with project filter |

## Build

### Requirements

- Qt 6.2+ (Widgets, Sql, LinguistTools)
- CMake 3.16+
- C++20

### Arch Linux

```bash
sudo pacman -S qt6-base cmake
```

### Ubuntu 22.04+

```bash
sudo apt install qt6-base-dev qt6-l10n-tools cmake
```

### Build

```bash
git clone <repo>
cd time_tracking
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/bin/time_tracking
```

## Project Structure

```
src/
├── main.cpp
├── data/
│   ├── models.h              # POD structs: Project, TimeEntry
│   ├── database.h/.cpp       # SQLite via Qt SQL
├── core/
│   └── timercontroller.h/.cpp  # Timer logic and auto-switching
├── ui/
│   ├── mainwindow.h/.cpp
│   ├── widgets/
│   │   ├── projectcardwidget   # Project card with color accent
│   │   ├── timerdisplay        # HH:MM:SS in the title bar
│   │   └── sessionhistorywidget  # Session history table
│   └── dialogs/
│       └── addprojectdialog    # New project dialog
└── resources/
    └── style.qss               # Dark theme (Catppuccin Mocha)
```

## Data Storage

SQLite database location:
- **Linux**: `~/.local/share/alex/TimeTracker/timetracker.db`
- **Windows**: `%APPDATA%\alex\TimeTracker\timetracker.db`

Settings (language etc.):
- **Linux**: `~/.config/alex/TimeTracker.conf`

## License

[MIT](LICENSE)
