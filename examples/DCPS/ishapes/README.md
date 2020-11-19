# ishapes

QT GUI application which has been adapted from
[The Simple DDS (SIMD) API](http://code.google.com/p/simd-cxx),
and can be used to demonstrate OpenDDS RTPS interoperability with other DDS
vendors.

The ishapes program is intended to be used alongside
[the Shapes Demo walkthrough on the OpenDDS website](http://opendds.org/quickstart/GettingStartedShapesDemo.html).
The examples there provide help to begin to familiarize one with some DDS
concepts by making use of the included application.

## Building

ishapes requires the core OpenDDS libraries so make sure you are able to build
them before attempting to build ishapes. It also uses Qt5.
To configure Qt5 applications in OpenDDS, please follow the
[OpenDDS Qt5 documentation (DDS\_ROOT/docs/qt.md)](../../../docs/qt.md).
Once configured, ishapes should be able to be built using the target/project
named `ishapes` but will also be built by default if everything is being
built.

## Usage

The ishapes program can accept a command line argument to specify the
partition to be used by this instance of the application by using:
`-partition VAL` where `VAL` should be replaced with the partition for instance
"A" or alternatively `*` can be used to specify inclusion in all partitions.

## Notes for Developers

The GUI source code files are compiled by the Qt meta-object compiler and
have certain macros and other non-standard language constructs that are
parsed by that system.

This means that it is not recommended to run a style utility on the GUI
files, as the Qt specific elements may no longer be recognizable by the
Qt tools.

### Files

  - `*.ui`
    - Qt designer files specifying the UI for the application.

  - `ishape.qrc`
    - Resource specification file, managed by Qt designer.

  - `ishape.mpc`
    - MPC project file for the application.

  - `*.{h,cpp}`
    - source code for application.
