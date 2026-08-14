cmake_minimum_required(VERSION 3.20)
project(lynxDB VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(NCURSESW REQUIRED ncursesw)
pkg_check_modules(MARIADB REQUIRED libmariadb)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

add_executable(lynxdb
    src/main.cpp
    src/app/App.cpp
    src/database/MySQLConnection.cpp
    src/database/SQLiteConnection.cpp
    src/database/DatabaseManager.cpp
    src/ui/Screen.cpp
    src/ui/Sidebar.cpp
    src/ui/TableView.cpp
    src/ui/StatusBar.cpp
    src/ui/ConnectionDialog.cpp
    src/ui/ErrorDialog.cpp
    src/ui/SqlQueryDialog.cpp
    src/ui/CellEditDialog.cpp
    src/ui/ConfirmDialog.cpp
)

target_include_directories(lynxdb PRIVATE
    src
    ${NCURSESW_INCLUDE_DIRS}
    ${MARIADB_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
)

target_link_libraries(lynxdb PRIVATE
    ${NCURSESW_LIBRARIES}
    ${MARIADB_LIBRARIES}
    ${SQLITE3_LIBRARIES}
)

target_compile_options(lynxdb PRIVATE
    ${NCURSESW_CFLAGS_OTHER}
    ${MARIADB_CFLAGS_OTHER}
    ${SQLITE3_CFLAGS_OTHER}
    -Wall -Wextra -Wpedantic
)

# Installation targets
install(TARGETS lynxdb DESTINATION bin)
install(FILES README.md LICENSE DESTINATION share/doc/lynxdb)

# CPack Packaging Configuration
set(CPACK_PACKAGE_NAME "lynxdb")
set(CPACK_PACKAGE_VENDOR "Venkata Najeer Kopparapu (github.com/Najeer-k11)")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "lynxDB — Fast, keyboard-driven Linux terminal database explorer for MySQL/MariaDB and SQLite")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "Venkata Najeer Kopparapu <https://github.com/Najeer-k11>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

# DEB (Debian/Ubuntu) packaging
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Venkata Najeer Kopparapu <https://github.com/Najeer-k11>")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libncursesw6, libmariadb3, libsqlite3-0")
set(CPACK_DEBIAN_PACKAGE_SECTION "database")

# RPM (Fedora/RHEL/SUSE/Nobara) packaging
set(CPACK_RPM_PACKAGE_REQUIRES "ncurses-libs, mariadb-connector-c, sqlite-libs")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_AUTOREQ 0)
set(CPACK_RPM_PACKAGE_AUTOREQPROV "no")

include(CPack)
