# Configuring Qt5 OpenDDS Applications

Some applications included with OpenDDS require Qt5:

  - [ishapes demo](../examples/DCPS/ishapes)
  - [OpenDDS Monitor](../tools/monitor)
  - [ExcelRTD](../tools/excelRTD)
    - (Indirectly through dependence on some of Monitor's functionality)

To build and use these applications, Qt5 must be available on the system and
Qt5 support must be enabled in the build system.

## Qt5

Qt5 is a framework primarily used for cross-platform GUI based applications.
The Qt official website as of writing is [qt.io](https://qt.io).
Qt can be downloaded prebuilt for all the major systems or as source code.

Also remember the options listed here are not the only ones.
Any functional instance of Qt5 would work as long as it is built and has the
headers and the basic Qt development tools (QtCreator is not required).

### Getting Qt Using Package Managers

Many package managers have Qt development packages available:

#### Linux

  - `qtbase5-dev` on Debian-based systems
  - `qt5-qtbase-devel` on Fedora-based systems
  - `qt5-base` on Arch

#### macOS

  - `qt5` using [brew](https://brew.sh/)
  - `qt5-qtbase` using [MacPorts](https://www.macports.org/)

#### Windows

The only package manager on Windows that appears to have a reliable Qt5
development package is [vcpkg](https://github.com/Microsoft/vcpkg) as
`qt5-base`. There is a `qt5` package that installs a complete Qt environment
(including `qt5-base`), but this is overkill for what OpenDDS needs and is not
required.

An important detail to note is that vcpkg requires a target
architecture when installing a package which they call a "triplet".
For 32-bit x86 Windows it is `x86-windows` which is the default and
for 64-bit x86 Windows it is `x64-windows`.
This must match the target architecture of OpenDDS.

To build Qt5 needed for 64-bit OpenDDS on 64-bit Windows:

```
vcpkg --triplet x64-windows install qt5-base
```

After that Qt root will be `installed\x64-windows` inside the root of vcpkg.

Also vcpkg works on Linux and macOS as well but building OpenDDS Qt
applications with Qt5 built with vcpkg has not been tested on Linux and macOS.

## Using the Configure Script

If using the configure script, Qt is enabled by using the `--qt` option. Other
arguments required depend on how Qt was acquired and where it's located on
the system.

### Using a Qt Installed Through the System Package Manager

If you're using a standard Linux distro (Debian-based, Red Hat-based, etc.),
and you want to use the system Qt (which is recommended), just supplying the
`--qt` option should be enough and the configure script will take care of the
rest. Should the configure script fail to recognize your Qt installation or
the build fails due to missing Qt files, you might have to supply Qt
parameters manually.

### Manually Specifying Qt Locations

If Qt isn't in a standard place and/or isn't installed, then Qt paths have to
be specified manually. You will need to supply the root of the Qt directory so
the build system can find the Qt tools, libraries, and headers. The configure
script has two options for Qt locations, `--qt` and `--qt-include`. If you
built your Qt from source, `--qt` should be set to the root of the results
or the install prefix. If headers were places some where other than
`include` or `include/qt5` of the root you have specified, you will need to
also specify their location using `--qt-include`.

#### Qt Build Parameters

These are the options that can be passed to configure how Qt applications are built.

| Env. Variable | Description           | Configure Script Option | Configure Script Default               |
| ------------- | --------------------- | ----------------------- | -------------------------------------- |
| `QTDIR`       | Qt Root Location      | `--qt`                  | `/usr`                                 |
| `QT5_INCDIR`  | Qt Header Location    | `--qt-include`          | `QTDIR/include` or `QTDIR/include/qt5` |
| `QT5_BINDIR`  | Qt Tools Location     | N/A                     | `QTDIR/bin`                            |
| `QT5_LIBDIR`  | Qt Libraries Location | N/A                     | `QTDIR/lib`                            |
| `QT5_SUFFIX`  | Qt Tools Name Suffix  | N/A                     | Blank or `-qt5`                        |

#### vcpkg

The configure script will try to detect if vcpkg is being used.  If there are
no conflicts with manually supplied values, it sets `QT5_BINDIR` to
`QTDIR/tools/qt5`. It will also set `QT5_LIBDIR` to `QTDIR/debug/lib` depending
on if `--debug` option has been pass to configure.

#### Windows Example

```
configure --qt=C:\Qt\5.11.1\msvc2017_64
```

This is for a prebuilt 64-bit Qt 5.11.1 for Visual Studio 2017 using the
official Qt Windows installer and the default location.

## Without the Configure Script

It is possible to build OpenDDS without using the configure script, although
this is not recommended unless one is familiar with OpenDDS and its build
system, MPC. To enable Qt5 support the `qt5` feature must be enabled (e.g.
`-features qt5=1`) and environment variables must be set to configure the build
to use Qt correctly:

| Env. Variable | Description           | MPC Default     |
| ------------- | --------------------- | --------------- |
| `QTDIR`       | Qt Root Location      | N/A (Required)  |
| `QT5_INCDIR`  | Qt Header Location    | `QTDIR/include` |
| `QT5_BINDIR`  | Qt Tools Location     | `QTDIR/bin`     |
| `QT5_LIBDIR`  | Qt Libraries Location | `QTDIR/lib`     |
| `QT5_SUFFIX`  | Qt Tools Name Suffix  | Blank           |
