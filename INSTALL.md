# Building and Installing OpenDDS

**Table of Contents:**

- [Building and Installing OpenDDS](#building-and-installing-opendds)
  - [Java](#java)
  - [Supported Platforms](#supported-platforms)
  - [Compiling](#compiling)
  - [Test](#test)
  - [Installation](#installation)
    - [Application Development with an Installed OpenDDS](#application-development-with-an-installed-opendds)
  - [Cross Compiling](#cross-compiling)
    - [Raspberry Pi](#raspberry-pi)
    - [Android](#android)
    - [Apple iOS](#apple-ios)
  - [Building Your Own Applications](#building-your-own-applications)

## Java

If you're building OpenDDS for use by Java applications, please see the file
[java/INSTALL](java/INSTALL) instead of this one.

## Supported Platforms

We have built OpenDDS on number of different platforms and compilers.  See
[README.md](README.md#supported-platforms) for a complete description of
supported platforms.

## Compiling

  OpenDDS has a "configure" script to automate all steps required before
  actually compiling source code.  This script requires Perl 5.10 or newer to be
  installed and available on the system PATH.  Perl 5.8 may be sufficient on
  Unix systems but [ActiveState Perl](https://www.activestate.com/products/activeperl/)
  5.10 or newer should be used on Windows.
  To start the script simply change to the directory containing this INSTALL
  file, and run:

**For Unixes (Linux, macOS, BSDs, etc):**

```
./configure
```

**For Windows (in a Visual Studio Command Prompt):**

```
configure
```

  Optionally add `--help` to the command line to see the advanced options
  available for this script.  The configure script will download ACE+TAO and
  configure it for your platform.  To use an existing ACE+TAO installation,
  either set the `ACE_ROOT` and `TAO_ROOT` environment variables or pass the `--ace`
  and `--tao` (if TAO is not at `$ACE_ROOT/TAO`) options to configure.
  If configure runs successfully it will end with a message about the next
  steps for compiling OpenDDS.

  OpenDDS supports parallel builds to speed up the build when using Make. To
  use this pass `-j N` where `N` the max number of parallel jobs to run. If not
  sure `N` should be, use the number of cores on the machine.

  The configure script creates an environment setup file called setenv (actually
  named `setenv.sh` or `setenv.cmd` depending on platform) that restores all the
  environment variables the build and test steps rely on.
  The main makefile for non-Windows builds temporarily sets the environment as
  well, so `setenv.sh` is not needed when running `make` from the top level.
  On Windows, the configure script modifies the environment of the command
  prompt that ran it. If using a new environment, use `setenv.cmd` to set the
  required environment variables before running Visual Studio.

## Test

**NOTE: Tests are not built by default, `--tests` must be passed to the
configure script.**

  Optionally, you can run the entire OpenDDS regression test suite with one
  Perl command.

  **NOTE:** Make sure your environment is set by checking the variable `DDS_ROOT`.
        Run setenv if it is not set.

**For Unixes (Linux, macOS, BSDs, etc):**

```
bin/auto_run_tests.pl
```

**For Windows:**

```
bin\auto_run_tests.pl
```

  If you built static libraries, add `-Config STATIC` to this command.
  To test RTPS features (uses multicast) add `-Config RTPS` to this command.
  On Windows if you build Release mode add `-ExeSubDir Release`.
  On Windows if you build static libraries add `-ExeSubDir Static_Debug`
  or `-ExeSubDir Static_Release`.


## Installation

  When OpenDDS is built using `make`, if the configure script was run with an
  argument of `--prefix=<prefix>` the `make install` target is available.

  After running `make` (and before `make install`) you have one completely ready
  and usable OpenDDS.  Its `DDS_ROOT` is the top of the source tree -- the same
  directory from which you ran configure and make.  That `DDS_ROOT` should work
  for building application code, and some users may prefer using it this way.

  After `make install` there is a second completely ready and usable OpenDDS
  that's under the installation prefix directory.  It contains the required
  libraries, code generators, header files, IDL files, and associated scripts
  and documentation.

### Application Development with an Installed OpenDDS

  After `make install` completes, the shell script in
  `<prefix>/share/dds/dds-devel.sh` is used to set the `DDS_ROOT` environment
  variable.  The analogous files for ACE and TAO are
  `<prefix>/share/ace/ace-devel.sh` and `<prefix>/share/tao/tao-devel.sh`.

  The `<prefix>` tree does not contain a tool for makefile generation.  To use
  MPC to generate application makefiles, the `MPC_ROOT` subdirectory from the
  OpenDDS source tree can be used either in-place or copied elsewhere.
  To use CMake to generate application makefiles,
  see [`docs/cmake.md`](docs/cmake.md).


## Cross Compiling

  Use the configure script, and set the target platform to one different than
  the host.  For example:

```
./configure --target=lynxos-178
```

  Run configure with `--target-help` for details on the supported targets.
  In this setup, configure will clone the OpenDDS and ACE+TAO source trees for
  host and target builds.  It will do a static build of the host tools (such as
  `opendds_idl` and `tao_idl`) in the host environment, and a full build in the
  target environment.  Most parameters to configure are then assumed to be
  target parameters.

  Any testing has to be done manually.


### Raspberry Pi

The instructions for building for the Raspberry Pi are on
[`opendds.org`](http://opendds.org/quickstart/GettingStartedPi.html).

### Android

Android support is documented in [`docs/android.md`](docs/android.md).

### Apple iOS

Apple iOS support is documented in [`docs/ios.md`](docs/ios.md).


## Building Your Own Applications

See the [OpenDDS Developer's Guide](
    http://download.ociweb.com/OpenDDS/OpenDDS-latest.pdf)
and run the Developer's Guide Example program:

**For Unixes (Linux, macOS, Solaris, BSDs, etc):**

```
cd $DDS_ROOT/DevGuideExamples/DCPS/Messenger
./run_test.pl
```

**For Windows:**

```
cd %DDS_ROOT%\DevGuideExamples\DCPS\Messenger
perl run_test.pl
```

  The Perl script will start 3 processes, the DCPSInfoRepo, one publisher, and
  one subscriber.  Note that the command lines used to spawn these processes
  are echoed back to standard output.  The options and config files used here
  are helpful starting points for developing and running your own OpenDDS
  applications.
