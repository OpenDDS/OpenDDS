# Configuring Qt5 OpenDDS Applications

Some included OpenDDS applications require Qt5:

  - ishapes demo
  - OpenDDS Monitor
  - ExcelRTD
    - (Indirectly through dependence on some of Monitor's functionality)

To build and use these applications, Qt5 must be available on the system and
Qt5 support must be enabled in the build system.

## Qt5

Qt5 is freely available from [qt.io](https://qt.io) or through your system's
package manager:

  - `qtbase5-dev` on Ubuntu
  - `qt5-qtbase-devel` on Fedora
  - Windows and macOS would probably be best served by downloading it from [qt.io](https://qt.io)

Any instance of Qt5 would work as long as it is built and has the headers
and basic Qt development tools.

## If Using the Configure Script

If using the configure script, supply the `--qt` option. If your Qt was
built from source or installed in a specific place (the case for prebuilt
binaries on Windows), you will need to supply the root of the Qt directory so
the build system can find the Qt tools, libraries, and headers. It's possible
for Qt to put in it's headers in a sub directory of the include directory,
so the location can be supplied with `--qt-include`. This is the case on
x86\_64 Ubuntu 18.04 with Qt5 installed from apt, which places them at
`/usr/include/x86_64-linux-gnu/qt5`.
In that case the options whould be
`--qt --qt-include /usr/include/x86_64-linux-gnu/qt5`
because the tools and libraries are in the standard locations but the headers
are not.

## Without the Configure Script

Even if not using the configure script, all of the same things apply except

  - The `qt5` feature must be enabled in MPC
  - `$QTDIR` must be set and corresponds to the `--qt` configure script option
  - `$QT5_INCDIR` corresponds to the `--qt-include` configure script option
