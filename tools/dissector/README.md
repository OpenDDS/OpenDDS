# OpenDDS DCPS Wireshark Dissector

The OpenDDS DCPS Wireshark dissector supports the TCP/IP, UDP/IP, and IP
multicast transports. Dissection of transport headers and encapsulated sample
headers are fully supported. Optional dissection of sample payloads is also
supported from Wireshark 1.12 on. The dissector is compatible with Wireshark
1.2 up to at least 3.0 and has been tested with Windows, macOS, and Linux.

If you need to dissect packets in RTPS DDS systems, all recent versions of
Wireshark have a built-in RTPS dissector. This dissector, at least for the
moment, just dissects OpenDDS systems that are NOT using RTPS. Once set up
though, this dissector allows viewing and filtering by dissected sample data
from within Wireshark.

## Table of Contents

- [See Also](#see-also)
- [Building](#building)
- [Usage](#usage)
  - [Environment Variables](#environment-variables)
  - [Display Filters](#display-filters)
  - [Color Filters](#color-filters)
- [Sample Dissection](#sample-dissection)
  - [Generating ITL Files](#sample-dissection-itl)
  - [Using Display Filters with Samples](#sample-dissection-filtering)
- [Known Limitations](#limitations)

<a name="see-also"></a>
## See also

- For Users:
  - [Wireshark Project Page](
      https://www.wireshark.org/)
  - [Wireshark Download Page](
      https://www.wireshark.org/download.html)
  - [Wireshark User's Guide](
      https://www.wireshark.org/docs/wsug_html_chunked/)
- For Developers Working on this Plugin:
  - [Wireshark Developer's Guide](
      https://www.wireshark.org/docs/wsdg_html_chunked/)
  - `doc/README.dissector` located in the Wireshark source
    - Has **very important** information not found in the Developer's Guide
  - [Wireshark Dissector API Doxygen](
      https://www.wireshark.org/docs/wsar_html/)

<a name="building"></a>
## Building

Follow the steps in the `INSTALL.md` file in the root of OpenDDS directory,
along with the steps here.

In addition, the Wireshark sources must be available and built locally
according to the [Wireshark Developer's Guide](https://www.wireshark.org/docs/wsdg_html_chunked/)
or the headers and libraries must be installed through a development package
like:

 - `wireshark-devel` on Fedora derived Linux distributions.
 - `wireshark-dev` on Debian derived Linux distributions.
 - `wireshark-cli` on Arch Linux. This is the base Wireshark package, so the
   development package is already installed if you installed the normal
   Wireshark package, `wireshark-qt`.

### Configure

To build the dissector, additional options must be passed to the configure
script before building OpenDDS, most importantly where to find Wireshark.

There are two ways to build against Wireshark depending on how Wireshark
was built or was acquired:

- The older method, `--wireshark` passed with the location of the
  Wireshark headers and libraries. This should be used with:

   - Any version of Wireshark built with autoconf, which is try if you
     ran `autogen.sh`. This is the recommended way to build on Linux before
     Wireshark 3.0.

   - Wireshark 1.x built on Windows. Also `%WIRETAP_VERSION%` should also
     be set to the version of wiretap in the build.

   - If there is a development package available for your system
     and you want to use it to avoid having to build Wireshark.
     This option defaults to `/usr/include/wireshark` if it exists, so
     supplying the path isn't necessary unless wireshark was installed
     else where.

- The newer out-of-source method, `--wireshark-cmake`, should be used if
  Wireshark was built using CMake, which can put "config.h" header and
  the Wireshark libraries somewhere other than the source tree. Before 3.0
  this was Wireshark's recommended method for Windows and macOS and can
  optionally be used on Linux. After 3.0 CMake is the only way to build
  Wireshark.
  CMake complicates things somewhat so two more options with arguments
  provided:

  - `--wireshark-build`
    - Is the build directory the user choose before building Wireshark.
      This is required and an absolute path.

  - `--wireshark-lib`
    - Is the location of the Wireshark libraries RELATIVE to the build
      location. It's optional but it might have to be supplied depending
      on the version of Wireshark as these defaults are based on
      Wireshark 2.4:
      - On Windows the default is `run\RelWithDebInfo`.
      - On macOS the default is `run/Wireshark.app/Contents/Frameworks`.
      - On Linux the default is an empty string as the libraries are in
        the build directory. For Wireshark 3.0 you should set this to `run`.

Glib is also required to build the dissector.
If Wireshark was not built with the system Glib or Glib is not installed,
the install prefix of Glib must be passed using `--glib`:
 - On Windows this will be something like: `wireshark-win(32|64)-libs-*\gtk2`
 - On macOS it depends on if the built-in dependency script or a package
   manager like Homebrew was used to install Wireshark's dependencies.
 - On the average Linux, it shouldn't be necessary unless you needed to use
   a Glib that is not installed (in `/usr`) to build Wireshark.

For optional sample payload dissection support, RapidJSON must be available.
It might already be available if OpenDDS was recursively cloned by a git client
or RapidJSON is installed in a default include location (RapidJSON is header
only library). If RapidJSON is not installed on the system, it must be
downloaded using: `git submodule update --init --recursive` or equivalent for
your git client. If OpenDDS isn't a git repository (such as if it was
downloaded from opendds.org) You can download RapidJSON source and pass it to
the configure script using `--rapidjson`.

### Build

Build normally as described in the `$DDS_ROOT/INSTALL.md` document.

### Install Plugin

To install the dissector plugin, copy the plugin file from
`DDS_ROOT/tools/dissector` to one of the plugin directories listed in the
Folders section in Wireshark's "About" dialog which can be found in the "Help"
menu. On Windows, the plugin file will be named `OpenDDS_Dissector.dll` or
`OpenDDS_Dissectord.dll` depending if OpenDDS was a Release or Debug Build.
On UNIX-like systems it will be named `OpenDDS_Dissector.so`.

See [Wireshark User's Guide Page on Plugins Folders](
https://www.wireshark.org/docs/wsug_html_chunked/ChPluginFolders.html)
for details on where Wireshark looks for plugins. This page, if working,
will be accurate for the latest stable version, but maybe not for previous
versions.

<a name="usage"></a>
## Usage

`setenv.sh` (or `setenv.cmd` for Windows) must be sourced before
running Wireshark or it will complain that it couldn't load the
dissector if the OpenDDS libraries are not installed system-wide.

You may verify the plugin is installed correctly by looking at the
"Supported Protocols" list. Depending on the Wireshark version, this
can usually be found somewhere under the "Help" or "Internals" menus.
The `OpenDDS_Dissector` library we created above should appear in the
list of plugins or protocols.

<a name="environment-variables"></a>
### Environment Variables

  - `OPENDDS_DISSECTORS`
    - If set the dissector will look for ITL files in that location
      to use for sample payload dissection.

  - There are two environment variables the dissector will use for debugging:

  - `OPENDDS_DISSECTOR_LOG`
    - If set, the dissector will write log output to `OpenDDS_wireshark.log` in
      the current directory.

  - `OPENDDS_DISSECTOR_SIGSEGV`
    - If set, the dissector will not handle segfaults when they occur
     during sample payload dissection. If not set, a segfault will
     cause a dialog to appear to suggest that the ITL file is wrong
     and close Wireshark. Setting this might be more helpful if
     using a debugger to see where the segfault occurred.
     Wireshark has a variable called `WIRESHARK_ABORT_ON_DISSECTOR_BUG` that
     is realted, but it has not been determined if how it relates exactly
     behaivoir wise.

<a name="display-filters"></a>
### Available Display Filters

A number of display filters are supported by the OpenDDS DCPS dissector:

#### Transport header display filters

  - `opendds.version`
    - Revision of the DCPS protocol.

  - `opendds.byte_order`
    - Byte order of transport header contents.

  - `opendds.length`
    - Total length of payload, including sample headers.

  - `opendds.sequence`
    - Sequence number of transmitted PDU.

  - `opendds.source`
    - Source identifier; only used by the multicast transport.

#### Sample header display filters

  - `opendds.sample.id`
    - Message ID of sample (i.e. `SAMPLE_DATA`).

  - `opendds.sample.sub_id`
    - Sub-Message ID of sample (i.e. `MULTICAST_SYN`).

  - `opendds.sample.flags`
    - Flags field (see below).

  - `opendds.sample.flags.byte_order`
    - Indicates byte order of sample data.

  - `opendds.sample.flags.coherent`
    - Indicates sample belongs to a coherent change group.

  - `opendds.sample.flags.historic`
    - Indicates sample is historic; resent by DataWriter.

  - `opendds.sample.flags.lifespan`
    - Indicates sample defines a lifespan.

  - `opendds.sample.flags.group_coherent`
    - Indicates sample uses `PRESENTATION.coherent_access`.

  - `opendds.sample.flags.content_filter`
    - Indicates the publisher has applied filters for Content-Filtered Topics.

  - `opendds.sample.length`
    - Total length of sample data, not including sample header.

  - `opendds.sample.sequence`
    - Sequence number of transmitted `SAMPLE_DATA`.

  - `opendds.sample.timestamp`
    - Source timestamp of sample.

  - `opendds.sample.lifespan`
    - Lifespan duration of sample (iff lifespan flag is set).

  - `opendds.sample.publication`
    - Publication ID of transmitting DataWriter.

  - `opendds.sample.publisher`
    - ID representing the coherent group (if `group_coherent` flag is set).

  - `opendds.sample.content_filter_entries`
    - Number of entries in this list for filtering (if `content_filter` flag is set).

See [below for Filtering based on the sample payload](#sample-dissection-filtering).

See the [Wireshark Wiki page on Display Filters](
  https://wiki.wireshark.org/DisplayFilters)
and the [Wireshark man page on Display Filters](
  https://www.wireshark.org/docs/man-pages/wireshark-filter.html)
for examples of how Wireshark display filters can be used.

<a name="color-filters"></a>
### Available Color Filters

A set of color filters are included in the source distribution which may
be imported to highlight DCPS protocol packets.

To import these filters, click on the View -> Coloring Rules menu item,
followed by the Import button.  Select the colorfilters file in the
`$DDS_ROOT/tools/dissector/` directory and click the Open button.

NOTE: Coloring rules are applied on a first match basis; you may need to
      move the imported rules above the "tcp" and "udp" rules using the
      Order buttons on the right-hand side of the dialog.  If ordering is
      changed, you must ensure the "OpenDDS (Important)" rule appears
      before the "OpenDDS" rule.

<a name="sample-dissection"></a>
## Sample Dissection

The dissector, when configured with RapidJSON, can dissect sample data on
Wireshark 1.12 and later. See the [section about building above](#building)
to enable sample dissection.

<a name="sample-dissection-itl"></a>
### Generating ITL files

ITL files are how the sample dissector knows the type names and structure
of the sample data. They must be generated from the same IDL files that
OpenDDS uses and must be updated when they are.

To create an ITL file from your IDL files:

1. Export type information using the OpenDDS IDL compiler.  The
   command `opendds_idl -Gitl <filename>.idl` will produce a file
   called `<filename>.itl` which contains the necessary type
   information.  The new MPC base project `gen_dissector` may be used
   in MPC to add `-Gitl` to the `opendds_idl` command line.

2. Start Wireshark in the directory containing the itl file or set the
   `OPENDDS_DISSECTORS` environment variable to the directory containing
   the itl file.  The OpenDDS Wireshark plugin will try to load all
   .itl files in either `OPENDDS_DISSECTORS` if specified or the current
   directory.

Type information is exported using Intermediate Type Language (ITL).
For more information, see
  [https://github.com/objectcomputing/OpenDDS/tree/master/tools/IntermediateTypeLang](
  https://github.com/objectcomputing/OpenDDS/tree/master/tools/IntermediateTypeLang)

<a name="sample-dissection-filtering"></a>
### Using Display Filters with Samples

When the dissector can dissect sample data, it also allows Wireshark to filter
OpenDDS packets based on contents.

The base of this filter namespace is `opendds.sample.payload`.
If the type `Messenger::Message` is being used by OpenDDS and the ITL
file is located in `OPENDDS_DISSECTORS` (or the directory Wireshark was started
in), then packets that carry this can be filtered by
`opendds.sample.payload.Messenger.Message`. Further more packets can be
filtered by the data in the payload, for example:

```
opendds.sample.payload.Messenger.Message.subject contains "DDS"
opendds.sample.payload.Messenger.Message.count >= 4
```

See the [Wireshark Wiki page on Display Filters](
  https://wiki.wireshark.org/DisplayFilters)
and the [Wireshark man page on Display Filters](
  https://www.wireshark.org/docs/man-pages/wireshark-filter.html)
for examples of how Wireshark display filters can be used.

#### Arrays and Sequences

IDL Arrays and Sequences value is the number of elements:

```
opendds.sample.payload.Messenger.Message.seq < 3
```

This will show all OpenDDS packets with a payload of type
`Messenger::Message` where it's `seq` member has less than 3 elements.

Because of how the Wireshark filter queries work it, it is not possible to
filter based on index of elements but elements inside the arrays and sequences
can be filtered using `_e` (short for **e**lement):
```
opendds.sample.payload.Messenger.Message.seq._e.some_value == 1
```

#### Enums

Values of Enums are represented by string names of the members. So if a
Enum has a member `Invalid`, then you can filter for packets with that
value with:

```
opendds.sample.payload.Messenger.Message.enum == "Invalid"
```

#### Unions

Union members are accessible by their name, not by their discriminator
value. If packets have a Union `u` on a boolean member `u_b` that is true,
then they can be found with:
```
opendds.sample.payload.Messenger.Message.u.u_b == 1
```

#### Strings and Characters

  - Wide strings and wide characters are converted from UTF-16 to UTF-8 using
    iconv. Data that is invalid UTF-16 is replaced by a message saying that
    it was invalid and what kind of problem iconv had with it.

  - To retain compatibility with Wireshark 1.12, OpenDDS/IDL Chars are given
    to Wireshark as strings and must be filtered as such. This means that when
    filtering they must be typed with double quotes (") like a C string
    literal, not single quotes (') like a C character literal. The same
    applies to WChars, but that's because Wireshark does not have a dedicated
    wide character type.

#### Numbers

  - Long Doubles are cast down to doubles.

<a name="limitations"></a>
## Known Limitations

  - As noted in the introduction, this dissector only works with OpenDDS
    systems which are not using RTPS.

  - OpenDDS only maintains wire compatibility with the current revision
    of the DCPS protocol.  This dissector is effective for the compiled
    protocol version only.

  - The sample dissector can not distinguish between two IDL types that
    have the same name. When using an ITL file that does not match
    what OpenDDS is using, the dissector will do one of following:
      - Cause Wireshark to crash, but a pop up window informs the user of the
        probable cause.
      - Mark the OpenDDS packet as malformed, which will be colored red.
      - Fill the sample payload fields with incorrect data, or omit fields
        entirely.
