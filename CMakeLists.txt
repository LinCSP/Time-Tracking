cmake_minimum_required(VERSION 3.16)
project(time_tracking VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql LinguistTools)

set(SOURCES
    src/main.cpp
    src/data/database.cpp
    src/core/timercontroller.cpp
    src/ui/mainwindow.cpp
    src/ui/widgets/projectcardwidget.cpp
    src/ui/widgets/timerdisplay.cpp
    src/ui/widgets/sessionhistorywidget.cpp
    src/ui/dialogs/addprojectdialog.cpp
)

qt_add_executable(time_tracking ${SOURCES})

qt_add_resources(time_tracking "resources"
    PREFIX "/"
    BASE_DIRS src/resources
    FILES src/resources/style.qss
)

target_include_directories(time_tracking PRIVATE src)

target_link_libraries(time_tracking PRIVATE
    Qt6::Widgets
    Qt6::Sql
)

qt6_add_translations(time_tracking
    TS_FILES src/translations/timetracker_ru.ts
    RESOURCE_PREFIX "/translations"
    NO_GENERATE_LUPDATE_TARGET
)

