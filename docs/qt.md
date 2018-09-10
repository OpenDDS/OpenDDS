# Configuring Qt5 OpenDDS Applications

Some applications included with OpenDDS require Qt5:

  - [ishapes demo](../examples/DCPS/ishapes)
  - [OpenDDS Monitor](../tools/monitor)
  - [ExcelRTD](../tools/excelRTD)
    - (Indirectly through dependence on some of Monitor's functionality)

To build and use these applications, Qt5 must be available on the system and
Qt5 support must be enabled in the build system.

## Qt5

Qt5 is a framework primarily used for cross-platform GUI based applications
and is freely available from [qt.io](https://qt.io) or through your
system's package manager:

  - `qtbase5-dev` on Ubuntu
  - `qt5-qtbase-devel` on Fedora
  - Windows and macOS would probably be best served by downloading it from [qt.io](https://qt.io)

Any instance of Qt5 would work as long as it is built and has the headers
and basic Qt development tools.

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
parameters manually. In this case read the next section.

### Manually Specifying Qt Locations

If Qt isn't in a standard place and/or isn't installed, then Qt paths have to
be specified manually. You will need to supply the root of the Qt directory so
the build system can find the Qt tools, libraries, and headers. The configure
script has two options for Qt locations, `--qt` and `--qt-include`. If you
built your Qt from source, `--qt` should be set to the root of the results
or the install prefix. If headers were places some where other than
`include` or `include/qt5` of the root you have specified, you will need to
also specify their location using `--qt-include`.

#### Windows Example

```configure --qt C:\Qt\5.11.1\msvc2017_64
```

This is for a prebuilt 64-bit Qt 5.11.1 for Visual Studio 2017, please change
according to the platform you are trying to use and if you installed Qt
somewhere else.

## Without the Configure Script

It is possible to build OpenDDS without using the configure script, although
this is not recommended unless one is familiar with OpenDDS and it's build
system, MPC. This list explains how the configure script tells MPC how to
build Qt5 applications, but not how to build OpenDDS without the configure
script in general.

  - The `qt5` feature must be enabled (e.g. `-features qt5=1`).
  - `QTDIR` environment variable points to the root of the Qt instance.
    - This is supplied to the configure script using the `--qt` option and
      defaults to `/usr` in the configure script.
  - `QT5_INCDIR` environment variable is optional and points to the location
    of the Qt headers.
    - MPC defaults to `QTDIR/include` if not defined.
    - This is supplied to the configure script using the `--qt-include` option
      and defaults to `/usr/include` or `/usr/include/qt5` in the configure
      script. This value is manulputed because the headers can be either
      location. This isn't an issue when using the configure script but should
      be noted if forgoing the configure script.
  - `QT5_BINDIR` environment variable is optional and points to the location
    of the Qt development tools.
    - MPC defaults to `QTDIR/bin` if not defined.
  - `QT5_LIBDIR` environment variable is optional and points to the location
    of the Qt libraries.
    - MPC defaults to `QTDIR/lib` if not defined.
