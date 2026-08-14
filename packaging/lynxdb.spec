Name:           lynxdb
Version:        0.1.0
Release:        1%{?dist}
Summary:        Fast, keyboard-driven Linux terminal database explorer

License:        MIT
URL:            https://github.com/Najeer-k11/lynxDB
Source0:        %{url}/archive/refs/tags/v%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  cmake >= 3.20
BuildRequires:  pkgconfig(ncursesw)
BuildRequires:  pkgconfig(libmariadb)
BuildRequires:  pkgconfig(sqlite3)

Requires:       ncurses-libs
Requires:       mariadb-connector-c
Requires:       sqlite-libs

%description
lynxDB is an information-dense terminal database explorer built with C++20,
CMake, and ncursesw. Supports MySQL/MariaDB and SQLite with schema structure
inspection, multiline SQL execution, OSC 52 clipboard copying, and single-cell editing.

%prep
%autosetup -n lynxDB-%{version}

%build
%cmake
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%doc README.md
%{_bindir}/lynxdb

%changelog
* Fri Aug 14 2026 Venkata Najeer Kopparapu <https://github.com/Najeer-k11> - 0.1.0-1
- Initial RPM release of lynxDB
