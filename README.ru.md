# ⏱ Time Tracker

Минималистичное приложение учёта рабочего времени на Qt6 с тёмной темой.

## Возможности

- **Проекты** — добавляй проекты с цветными метками
- **Автопереключение** — при переходе на другой проект текущий таймер останавливается автоматически
- **История сессий** — журнал всех рабочих сессий с фильтром по проекту
- **Персистентность** — данные хранятся в SQLite, активная сессия восстанавливается при перезапуске
- **Мультиязычность** — English / Русский, переключение сохраняется между сеансами
- **Кастомное окно** — frameless-окно с перетаскиванием за шапку, двойной клик — максимизировать

## Скриншоты

| Проекты | История |
|---------|---------|
| Тёмная тема с цветными карточками проектов | Таблица сессий с фильтром по проекту |

## Сборка

### Зависимости

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

### Сборка

```bash
git clone <repo>
cd time_tracking
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/bin/time_tracking
```

## Структура проекта

```
src/
├── main.cpp
├── data/
│   ├── models.h              # POD-структуры Project, TimeEntry
│   ├── database.h/.cpp       # SQLite через Qt SQL
├── core/
│   └── timercontroller.h/.cpp  # Логика таймера, автопереключение
├── ui/
│   ├── mainwindow.h/.cpp
│   ├── widgets/
│   │   ├── projectcardwidget   # Карточка проекта с цветной полосой
│   │   ├── timerdisplay        # HH:MM:SS в шапке
│   │   └── sessionhistorywidget  # Таблица истории
│   └── dialogs/
│       └── addprojectdialog    # Диалог создания проекта
└── resources/
    └── style.qss               # Тёмная тема (Catppuccin Mocha)
```

## Хранение данных

База SQLite располагается в:
- **Linux**: `~/.local/share/alex/TimeTracker/timetracker.db`
- **Windows**: `%APPDATA%\alex\TimeTracker\timetracker.db`

Настройки (язык и др.):
- **Linux**: `~/.config/alex/TimeTracker.conf`

## Лицензия

[MIT](LICENSE)
