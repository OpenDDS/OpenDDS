# OpenDDS and Android

**How to build OpenDDS for Android and incorporate OpenDDS into Android apps.**

* [Variables](#variables)
* [Requirements](#requirements)
* [Building on Windows](#building-on-windows)
* [Building OpenDDS for Android](#building-opendds-for-android)
  * [Using the NDK Directly](#using-the-ndk-directly)
  * [Using a Standalone Toolchain](#using-a-standalone-toolchain)
  * [Configure Script Macros](#configure-script-macros)
    * [`android_ndk`](#android_ndk)
    * [`android_sdk`](#android_sdk)
    * [`android_abi`](#android_abi)
    * [`android_api`](#android_api)
    * [`android_target_api`](#android_target_api)
  * [Host Tools](#host-tools)
  * [OpenDDS's Optional Dependencies](#openddss-optional-dependencies)
    * [Java](#java)
    * [Android SDK](#android-sdk)
    * [OpenSSL](#openssl)
    * [Xerces](#xerces)
* [Cross-Compiling IDL Libraries](#cross-compiling-idl-libraries)
  * [Java IDL Libraries](#java-idl-libraries)
* [Using OpenDDS in a Android App](#using-opendds-in-a-android-app)
  * [Adding the OpenDDS Native Libraries to the App](#adding-the-opendds-native-libraries-to-the-app)
  * [Adding OpenDDS Java Libraries to the App](#adding-opendds-java-libraries-to-the-app)
  * [Network Permissions and Availability](#network-permissions-and-availability)
    * [Network Availability](#network-availability)
  * [OpenDDS Configuration Files](#opendds-configuration-files)
  * [Multithreading](#multithreading)
  * [Android Activity Lifecycle](#android-activity-lifecycle)
* [Footnotes](#footnotes)

## Variables

The following table describes some of the variables of paths that are
referenced in this guide. You don't have to actually set and use these, they
are mostly for shorthand.

| Variable     | Description                                      |
| ------------ | ------------------------------------------------ |
| `$DDS_ROOT`  | OpenDDS being built for Android                  |
| `$HOST_DDS`  | [OpenDDS host to help build OpenDDS for Android](#host-tools) |
| `$ACE_ROOT`  | ACE being built for Android                      |
| `$NDK`       | The Android NDK                                  |
| `$TOOLCHAIN` | The Generated Toolchain                          |
| `$SDK`       | The Android SDK (By default `$HOME/Android/Sdk`) |
| `$STUDIO`    | Android Studio                                   |
| `$JDK`       | The Java SDK                                     |
| `$SSL_ROOT`  | Install prefix for cross-compiled OpenSSL        |
| `$ABI`       | [`android_abi` value](#abi-table)                |
| `$ARCH_ARG`  | [Target architecture argument for some commands](#abi-table) |
| `$MIN_API`   | The Minimum Android API Version Number           |
| `$TARGET_API`| The Target API Version of your build             |

## Requirements

To build the core OpenDDS native libraries for Android you will need:

 - A development system supported by both OpenDDS and the Android NDK.
   - Windows and Linux were tested, but macOS should work as well.
 - [Android Native Development Kit (NDK)](https://developer.android.com/ndk/)
   r18 or higher. [Building with the NDK directly](#using-the-ndk-directly)
   requires NDK r19 or higher. You can download it separately from android.com
   or using the SDK Manager that comes with Android Studio. If you downloaded
   the NDK using the SDK Manager, then it is located at `$SDK/ndk-bundle`.
 - Some knowledge about OpenDDS and Android development will be assumed, but
   more OpenDDS knowledge will be assumed than Android knowledge.
 - Windows users should see ["Building on Windows"](#building-on-windows) for
   additional requirements they might need.

In addition to those, building OpenDDS with optional dependencies also have
additional requirements listed in their own sections.

The [\"Using OpenDDS in a Android App\"
section](#using-opendds-in-a-android-app) assumes the use of Android Studio,
but it will also work when just using the Android SDK if tweaked.

## Building on Windows

If using you're using Windows Subsystem for Linux (WSL), Docker, or anything
like that, then when you're done with the guide you can copy the resulting
libraries from the virtual environment to Windows and where they can be used in
Android Studio as they would be used on Linux. Also you can skip the rest of
this section and any **Windows Users:** notices.

If you want to build OpenDDS for Android on Windows without WSL or Docker,
follow the **Windows Users:** notices.

In addition to OpenDDS and the Android NDK you will also need the following
software:

- [MSYS2](https://www.msys2.org)
  - Building OpenDDS and its dependencies for Android requires various
    utilities that would normally come on a Unix system. This guide will use
    MSYS2, which supplies many of those utilities. Install MSYS2 from the
    official website at https://www.msys2.org and set it up.
  - Follow all the install/update steps from the msys2.org website.

- [Strawberry Perl](http://strawberryperl.com/)

- OpenDDS Host tools build using Visual Studio
  - In a separate copy of OpenDDS, build OpenDDS as described in
    [`INSTALL.md`](../INSTALL.md) using Visual Studio, except use the
    `--host-tools-only` configure script option.  This OpenDDS (and the ACE+TAO
    it uses) must be the same version as the one used to build for Android.
  - If you want to use Java in the Android build, also pass the
    `--java` configure script option here as described in the [Java](#java)
    section. You will also need to pass it to the configure script build for
    Android when that comes.

Finally, all paths being passed to GNU Make must not contain spaces because
ACE's gnuace make scripts don't handle those paths correctly on Windows. This
means the NDK, toolchain, MSYS2, JDK, OpenDDS source, OpenDDS host tools, etc.
must not contain any spaces in their paths.

## Building OpenDDS for Android

As Android targets multiple architectures and has many versions, an
architecture and minimum API version to use will have to be decided. As of
writing [this page lists Android version numbers and their corresponding API
versions](https://source.android.com/setup/start/build-numbers). You will have
to do a separate build for each architecture if you want to build OpenDDS for
multiple architectures.

OpenDDS for Android can be built in two ways:
[using the NDK directly](using-the-ndk-directly) or
[using a standalone toolchain](using-the-ndk-directly).

Using the NDK directly is recommended by Google and means that toolchains don't
have to be generated for each target architecture.

### Using the NDK Directly

**Building with the NDK directly requires NDK r19 or later.**

OpenDDS can be configured and built with the Android NDK using the following
commands.

**NOTE**: If you need to configure OpenDDS with any optional dependencies then
read the [relevant sections](#openddss-optional-dependencies) before
configuring and building OpenDDS.

```Shell
./configure --doc-group3 --target=android --macros=android_abi=$ABI --macros=android_api=$MIN_API --macros=android_ndk=$NDK
make # Pass -j/--jobs with an appropriate value or this'll take a while...
```

### Using a Standalone Toolchain

To build OpenDDS with with a Android standalone toolchain, a standalone
toolchain must first be generated by using:

```Shell
$NDK/build/tools/make_standalone_toolchain.py --arch $ARCH_ARG --api $MIN_API --install-dir $TOOLCHAIN
```

**Windows Users:** Android NDK includes Python in `prebuilt\windows-x86_64\bin`
for 64-bit Windows NDKs. For the example above, assuming `%NDK%` is the
location of the NDK and `%TOOLCHAIN%` is the desired location of the toolchain,
run this command instead:

```bat
%NDK%\prebuilt\windows-x86_64\bin\python %NDK%\build\tools\make_standalone_toolchain.py --arch %ARCH_ARG% --api %MIN_API% --install-dir %TOOLCHAIN%
```

The `--arch` argument for `make_standalone_toolchain.py` and
`--macros=ANDROID_ABI=<ARCH>` argument for the configure script must match
according to [this table](#abi-table).

**NOTE**: If you need to configure OpenDDS with any optional dependencies then
read the [relevant sections](#openddss-optional-dependencies) before
configuring and building OpenDDS.

```Shell
./configure --target=android --macros=ANDROID_ABI=$ABI
PATH=$PATH:$TOOLCHAIN/bin make # Pass -j/--jobs with an appropriate value or this'll take a while...
```

**Windows Users:** The commands for this are similar with these exceptions:

- `--host-tools` with the location of the OpenDDS host tools that were built
  using Visual Studio must be passed to `configure`.

- You will need MSYS2 utilities in your `%PATH%`.

- Run these commands in a new Visual Studio command prompt that is
  different from where you configured the host tools.

```Batch
configure --target=android --macros=ANDROID_ABI=%ABI% --host-tools=%HOST_DDS%
set PATH=%PATH%;%TOOLCHAIN%\bin;C:\msys64\usr\bin
make
REM Pass -j/--jobs with an appropriate value or this'll take a while...
```

### Configure Script Macros

#### `android_ndk`

Location of Android NDK, Same as [`$NDK`](#variables).
This is required when [building with the NDK
directly](#using-the-ndk-directly), but not when building with a standalone
toolchain.

#### `android_sdk`

Location of Android SDK, same as [`$SDK`](#variables). This is only required if
enabling OpenDDS to use Android Java APIs.

#### `android_abi`

The architecture to cross-target. When using ACE6/TAO2 `ANDROID_ABI` must be
used and this is optional as it defaults to `armeabi-v7a`. When using ACE7/ACE3
either `ANDROID_ABI` or `android_abi` can be used, but it is required. The
valid options are:

<a id="abi-table"/>

| `$ARCH_ARG`| `android_abi`           | `$ABI_PREFIX`           | Description                         |
| ---------  | ----------------------- | ----------------------- | ----------------------------------- |
| `arm`      | `armeabi-v7a`           | `arm-linux-androideabi` | 32-bit ARM                          |
| `arm`      | `armeabi-v7a-with-neon` | `arm-linux-androideabi` | 32-bit ARM with [NEON](#footnote-1) |
| `arm64`    | `arm64-v8a`             | `aarch64-linux-android` | 64-bit ARM                          |
| `x86`      | `x86`                   | `i686-linux-android`    | 32-bit x86                          |
| `x86_64`   | `x86_64`                | `x86_64-linux-android`  | 64-bit x86                          |

#### `android_api`

The minimum Android API to target. This is the same as [`$MIN_API`](#variables)
This is required when [building with the NDK
directly](#using-the-ndk-directly), but not when building with a standalone
toolchain.

#### `android_target_api`

The Android API being targeted by an application. This is the same as
[`$TARGET_API`](#variables). This is only required if enabling OpenDDS to use
Android Java APIs.

### Host Tools

To cross-compile OpenDDS, host tools are required to process IDL. The example
above generates two copies of OpenDDS, one in `OpenDDS/build/host` and another
in `OpenDDS/build/target`. If this is the case, then `$HOST_DDS` will be the
absolute path to `build/host` and `$DDS_ROOT` will be the absolute path to
`build/target`.

If building for more than one architecture, which will be necessary to cover
the largest number of Android devices possible, it might make sense to build
the OpenDDS host tools separately to cut down on compile time and disk space.

If this is the case, then `$HOST_DDS` will be the location of the static host
tools built for the host platform and `$DDS_ROOT` will just be the location of
the OpenDDS source code.

This should be done with the same version of OpenDDS and ACE/TAO as what you
want to build for Android. Pass `--host-tools-only` to the configure script to
generate static host tools. Also pass `--java $JDK` if you plan on using Java.

If you want to just the minimum needed for host OpenDDS tools and get rid of
the rest of the source files, you can. These are the binaries that make up the
OpenDDS host tools:

 * `$HOST_DDS/bin/opendds_idl`
 * `$HOST_DDS/bin/idl2jni` (if using the OpenDDS Java API)
 * `$HOST_DDS/ACE_TAO/bin/ace_gperf`
 * `$HOST_DDS/ACE_TAO/bin/tao_idl`

These files can be separated from the rest of the OpenDDS and ACE/TAO source
trees, but the directory structure must be kept. To use these to build OpenDDS
for Android, pass `--host-tools $HOST_DDS` to the configure script.

### OpenDDS's Optional Dependencies

#### Java

To use OpenDDS in the traditional Android development language, Java, you will
need to build the Java bindings when building OpenDDS. See
[../java/README](../java/README) for details. For Android you can use the JDK
provided with Android Studio, so `JDK=$STUDIO/jre`. Pass `--java=$JDK` to the
OpenDDS configure script.

#### Android SDK

OpenDDS can make use of Android's Java SDK. Right now this is just used for
allowing OpenDDS to always be notified of [network availability when targeting
API 30 and later](#network-availability)

#### OpenSSL

OpenSSL is required for OpenDDS Security. Fortunately it comes with a
configuration for Android, please read `NOTES.ANDROID` that comes with
OpenSSL's source code for how to build OpenSSL for Android.

Android preloads the system SSL library (either OpenSSL or BoringSSL) for the
Java Android API, so OpenSSL **MUST** be statically linked to the OpenDDS
security library. The static libraries will used if the shared libraries are
not found. This can be accomplished by either disabling the generation of the
shared libraries by passing `no-shared` to OpenSSL's `Configure` script or just
deleting the `so` files after building OpenSSL.

Cross-compiling OpenSSL on Windows:
1. Start the MSYS2 MSYS development shell using the start menu shortcut or `C:\msys64\msys2_shell.cmd -msys`
2. `cd /c/your/location/of/OpenSSL-source`
3. `export ANDROID_NDK_HOME=/c/your/location/of/ndk-standalone-toolchain`
4. `PATH+=:$ANDROID_NDK_HOME/bin`
5. `./Configure --prefix=$SSL_ROOT android-arm no-tests no-shared` (or replace -arm with a different platform like -arm64, see OpenSSL's NOTES.ANDROID file)
6. `make install_sw`

#### Xerces

Xerces C++ is also required for OpenDDS Security. It does not support Android
specifically, but it comes with a CMake build script that can be paired with
the Android NDK's CMake toolchain.

Xerces requires a supported "transcoder" library. For API levels greater than
or equal to 28 one of these, GNU libiconv, is included with Android. Before 28
any of the transcoders supported by Xerces would work theoretically but GNU
libiconv was the one tested.  If GNU libiconv is used, build it as an archive
library (`--disable-shared`) so that the users of Xerces (ACE and OpenDDS) don't
need to be aware of it as an additional runtime dependency.

Download [GNU libiconv](https://ftp.gnu.org/pub/gnu/libiconv) version 1.16
source code and extract the archive.

Cross-compiling GNU libiconv on Windows:
1. Start the MSYS2 MSYS development shell using the start menu shortcut or `C:\msys64\msys2_shell.cmd -msys`
2. `cd /c/your/location/of/libiconv-source`
3. `export ANDROID_NDK_HOME=/c/your/location/of/ndk-standalone-toolchain`
4. `PATH+=:$ANDROID_NDK_HOME/bin`
5. `target=arm-linux-androideabi` (or select a different NDK target)
6. `./configure --disable-shared --prefix=/c/your/location/of/installed-libiconv --host=$target CC=$target-clang CXX=$target-clang++ LD=$target-ld CFLAGS="-fPIE -fPIC" LDFLAGS=-pie`
7. `make && make install`

Note that the directory given by `--prefix=` will be created by `make install`
and will have `include` and `lib` subdirectories that will be used by the Xerces
build.

Cross-compiling Xerces on Windows, using GNU libiconv:

A modified version of Xerces C++ hosted on
[OCI Labs on GitHub](https://github.com/oci-labs/xerces-c/tree/android)
has support for an external GNU libiconv.  Download this version using git or
the GitHub web interface [ZIP archive](https://github.com/oci-labs/xerces-c/archive/android.zip).
Note that this is the `android` branch of the repository.

Start the Microsoft Visual Studio command prompt for C++ development (for
example "x64 Native Tools Command Prompt for VS 2019").

`cmake` and `ninja` should be on the PATH.  They can be installed as on option
component in the Visual Studio installer (see "C++ CMake tools for Windows"),
or downloaded separately.

Set environment variables based on the NDK location and Android configuration selected:
1. `set target=arm-linux-androideabi`
2. `set abi=armeabi-v7a`
3. `set api=16`
4. `set NDK=C:\your\location\of\NDK`
5. `set GNU_ICONV_ROOT=C:\your\location\of\installed-libiconv`

Configure and build with CMake
1. `cd C:\your\location\of\Xerces-for-android`
2. `mkdir build & cd build`
3. `cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=C:\your\location\of\installed-xerces -DCMAKE_TOOLCHAIN_FILE=%NDK%\build\cmake\android.toolchain.cmake -DANDROID_ABI=%abi% -DANDROID_PLATFORM=android-%api% "-DANDROID_CPP_FEATURES=rtti exceptions" ..`
4. `cmake --build . --target install`

## Cross-Compiling IDL Libraries

Like all OpenDDS applications, you will need to use type support libraries
generated from IDL files to use most of OpenDDS's functionality.

Assuming the library is already setup and works for a desktop platform, then
you should be able to run:

```Shell
(source $DDS_ROOT/setenv.sh; mwc.pl -type gnuace && PATH=$PATH:$TOOLCHAIN/bin make)
```

The resulting native IDL library file must be included with the rest of the
native library files.

### Java IDL Libraries

Java support for your IDL, assuming OpenDDS was built with Java, will available
by inheriting `dcps_java` in your IDL MPC project and will be built along with
the native IDL libraries using the command above.

Java IDL libraries consist of two components: a Java `jar` library file and a
supporting native library `so` file. This native library must be included with
the other native library files, and is different than the regular native IDL
type support library.

## Using OpenDDS in a Android App

After building OpenDDS and generating the IDL libraries, you will need to set
up an app to be able to use OpenDDS.

There is an OpenDDS demo for using OpenDDS over the Internet that includes an
Android app built using these instructions:
[github.com/oci-labs/opendds-smart-lock](https://github.com/oci-labs/opendds-smart-lock).

### Adding the OpenDDS Native Libraries to the App

In your app's `build.gradle` (*NOT THE ONE OF THE SAME NAME IN THE ROOT OF THE
PROJECT*) add this to the `android` section:

```Gradle
    sourceSets {
        main {
            jniLibs.srcDirs 'native_libs'
        }
    }
```

`native_libs` is not a required name, but it needs to contain subdirectories
named after the `android_abi` of the native libraries it contains (See
the [ABI/architecture table](#abi-table)).

The exact list of libraries to include depend on what features you're using but
the basic list of library file for OpenDDS are as follows:

  - Core OpenDDS library and its dependencies:

    - If not already included because of a separate C++ NDK project, you must
      include the Clang C++ Standard Library. This is located at:
      - Standalone toolchain:
        `$TOOLCHAIN/sysroot/usr/lib/$ABI_PREFIX/libc++_shared.so`
      - NDK:
        `$NDK/toolchains/llvm/prebuilt/$HOST_PLATFORM/sysroot/usr/lib/$ABI_PREFIX/libc++_shared.so`
      - `$ABI_PREFIX` is an identifier for the architecture whose possible
        values can be found in the [ABI/architecture table](#abi-table).

    - `$ACE_ROOT/lib/libACE.so`
    - `$ACE_ROOT/lib/libTAO.so`
    - `$DDS_ROOT/lib/libOpenDDS_Dcps.so`

 - The following are the transport libraries, one for each transport type. You
   will need at least one of these, depending on the transport(s) you want to
   use:
   - `$DDS_ROOT/lib/libOpenDDS_Rtps_Udp.so`
     - Depends on `$DDS_ROOT/lib/libOpenDDS_Rtps.so`
   - `$DDS_ROOT/lib/libOpenDDS_Multicast.so`
   - `$DDS_ROOT/lib/libOpenDDS_Shmem.so`
   - `$DDS_ROOT/lib/libOpenDDS_Tcp.so`
   - `$DDS_ROOT/lib/libOpenDDS_Udp.so`

 - The [type support libraries for your IDL](#cross-compiling-idl-libraries)

 - The following are the Discovery libraries. Static discovery is built
   into `libOpenDDS_Dcps.so`, but most likely you will want one of these:

   - Required to use RTPS Discovery:
     - `$DDS_ROOT/lib/libOpenDDS_Rtps.so`

   - Required to use the DCPSInfoRepo Discovery:
     - `$DDS_ROOT/lib/libOpenDDS_InfoRepoDiscovery.so`
       - Depends on:
         - `$ACE_ROOT/lib/libTAO_PortableServer.so`
         - `$ACE_ROOT/lib/libTAO_AnyTypeCode.so`
         - `$ACE_ROOT/lib/libTAO_BiDirGIOP.so`
         - `$ACE_ROOT/lib/libTAO_CodecFactory.so`
         - `$ACE_ROOT/lib/libTAO_PI.so`

 - Required to use OpenDDS Security:
   - `$ACE_ROOT/lib/libACE_XML_Utils.so`
   - `libxerces-c-3.*.so`
   - `libiconv.so` if it is necessary to include it.
   - `$DDS_ROOT/lib/libOpenDDS_Security.so`

 - In addition to the jars listed below, the following native libraries are
   required for using the Java API:
   - `$DDS_ROOT/lib/libtao_java.so`
   - `$DDS_ROOT/lib/libidl2jni_runtime.so`
   - `$DDS_ROOT/lib/libOpenDDS_DCPS_Java.so`
     - Depends on:
       - `$DDS_ROOT/lib/libOpenDDS_Rtps_Udp.so`
       - `$DDS_ROOT/lib/libOpenDDS_Rtps.so`
       - `$DDS_ROOT/lib/libOpenDDS_Tcp.so`
       - `$DDS_ROOT/lib/libOpenDDS_Udp.so`
       - `$ACE_ROOT/lib/libTAO_PortableServer.so`
       - `$ACE_ROOT/lib/libTAO_AnyTypeCode.so`
       - `$ACE_ROOT/lib/libTAO_BiDirGIOP.so`
       - `$ACE_ROOT/lib/libTAO_CodecFactory.so`
       - `$ACE_ROOT/lib/libTAO_PI.so`
   - The [native part of the Java library for your IDL libraries](
      #java-idl-libraries)

This list might not be complete, especially if you're using a major feature not
listed here.

### Adding OpenDDS Java Libraries to the App

In your app's `build.gradle` (*NOT THE ONE OF THE SAME NAME IN THE ROOT OF THE
PROJECT*) add this to the `dependencies` section if not already there:

```Gradle
    implementation fileTree(include: ['*.jar'], dir: 'libs')
```

Copy these jar files from `$DDS_ROOT/lib` to a directory called `libs` in your
app's subdirectory. Create `libs` if it doesn't exist. Like `native_libs` the
`libs` name isn't required.

 - `i2jrt.jar`
 - `i2jrt_corba.jar`
 - `OpenDDS_DCPS.jar`
 - `tao_java.jar`
 - The [Java part of the Java library for your IDL](#java-idl-libraries)

Also copy the jar files from your IDL Libraries and sync with Gradle if you're
using Android Studio. After this OpenDDS Java API should be able to be used
the same as if using OpenDDS with the Hotspot JVM. The exceptions and
particulars to how Android can effect OpenDDS are described in the following
sections.

### Network Permissions and Availability

In `AndroidManifest.xml` you will need to add the network permissions if they
are not already there:

```XML
    <uses-permission android:name="android.permission.INTERNET" />
    <uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
```

Failure to do so will result in ACE failing to access any sockets and OpenDDS
will not be able to function.

#### Network Availability

When not running in an application targeting API 30 later on Android 10 or
later, Android builds of OpenDDS use the `LinuxNetworkConfigMonitor` to
reconfigure OpenDDS connections automatically when the device switches from one
network (cellular or wifi) to another.

When running in an application targeting API 30 later on Android 10,
`LinuxNetworkConfigMonitor` can no longer be used, as Netlink sockets are
blocked by OS for security reasons. In the logs this error show up as:

```
ERROR: LinuxNetworkConfigMonitor::open: could not open socket: Permission denied
```

Instead, `NetworkConfigModifier` is utilized. As a consequence of this, two new
variables are required from the user, `android_sdk`, and `android_target_api`.
These correspond to the location of your Android SDK, likely
`$HOME/Android/Sdk` on Linux, and the API number you are targeting. The
`NetworkConfigModifier` is set up along with the necessary network callbacks
when the user uses `TheParticipantFactory.WithArgs`. Pass the following options
to the `configure` script make this possible:

```Shell
--macros=android_sdk=$SDK --macros=android_target_api=$TARGET_API --java
```

### OpenDDS Configuration Files

OpenDDS can use several types of files: a main configuration file, security
configuration files, and security certificate files, among others. On
traditional platforms, distributing and reading these files is usually not an
issue at all. On Android however, an app has no traditional files of it's own
out of the box, so you can't give OpenDDS a path to a file you want to
distribute with the app without preparing beforehand.

If you already have a preferred way to include files in your app, then that
will work as long as you can give OpenDDS the path to the files.

Android can open a file stream for resource and asset files. Ideally OpenDDS
would be able to accept these streams, but it doesn't. One solution to this is
reading the streams into memory and then writing them to files in the app's
private directory. This example is using assets, but resources will also work
with some slight modifications.

```Java
// ...
    private String copyAsset(String asset_path) {
        File new_file = new File(getFilesDir(), asset_path);
        final String full_path = new_file.getAbsolutePath();
        try {
            InputStream in = getAssets().open(asset_path, AssetManager.ACCESS_BUFFER);
            byte[] buffer = new byte[in.available()];
            in.read(buffer);
            in.close();
            FileOutputStream out = new FileOutputStream(new_file);
            out.write(buffer);
            out.close();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return full_path;
    }
// ...

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // ...

        final String config_file = copyAsset("opendds_config.ini");
        String[] args = new String[] {"-DCPSConfigFile", config_file};
        StringSeqHolder argsHolder = new StringSeqHolder(args);
        dpf = TheParticipantFactory.WithArgs(argsHolder);
        // ...
    }
// ...
```

This example works but in production code the error handling should be improved
and integrated with the app's initialization. Rewriting the file every time is
not ideal, but OpenDDS's files are small and this method ensures the files are
up-to-date.

### Multithreading

When using a `DataReaderListener`, the callbacks will be using a ACE reactor
worker thread, which can't make changes to the Android GUI directly because
it's not the main thread. To have these callbacks affect changes in the Android
GUI, use something like
[`android.os.Handler`](https://developer.android.com/reference/android/os/Handler):

```Java
// ...

import android.os.Handler;

// ...

public class DataReaderListenerImpl extends DDS._DataReaderListenerLocalBase {

    private MainActivity context;

    public DataReaderListenerImpl(MainActivity context) {
        super();
        this.context = context;
    }

    public synchronized void on_data_available(DDS.DataReader reader) {
        StatusDataReader mdr = StatusDataReaderHelper.narrow(reader);
        if (mdr == null) {
            return;
        }
        StatusHolder mh = new StatusHolder(new Status());
        SampleInfoHolder sih = new SampleInfoHolder(new SampleInfo(0, 0, 0,
                new DDS.Time_t(), 0, 0, 0, 0, 0, 0, 0, false, 0));
        int status = mdr.take_next_sample(mh, sih);

        if (status == RETCODE_OK.value) {

            // ...

            Handler handler = new Handler(context.getMainLooper());
            handler.post(new Runnable() {
                @Override
                public void run() {
                    context.tryToUpdateThermostat(thermostat_status);
                }
            });
        }
    }
}
```

### Android Activity Lifecycle

The [Android Activity Lifecycle](https://developer.android.com/guide/components/activities/activity-lifecycle)
is something that affects all Android apps. In the
case of OpenDDS, the interaction gets more complicated because of the
intersection of the similar, but distinct process lifecycle. The process hosts
the activity, but isn't guaranteed to be kept alive after `onStop()` is called.
What makes this worse for NDK applications is that there doesn't seem to be a
way to be warned of the killing of the process the way Java application can
rely on `onDestroyed()`. For most OpenDDS applications, this isn't a
serious issue.

An easy way to make sure participants are cleaned up is to create participants in
`onStart()` as might be expected, and always delete them in `onStop()`, so
that they may be created again in `onStart()`. The `DomainParticpantFactory`
can be retrieved either in `onStart()` or more perhaps appropriately in
[`Application.onStart()`](https://developer.android.com/reference/android/app/Application),
given the singleton nature of both.

This might not be ideal or efficient though, because deleting and recreating
participants will happen every time the app loses focus, like during
orientation changes. An alternative to this is to run OpenDDS within an
[Android Service](https://developer.android.com/guide/components/services)
separate from the main app with the service configured so that it does not
stopped when the Application's `onStop()` is called. The service should be
specified in `AndroidManifest.xml`.

```XML
    <service
            android:name=".OpenDdsService"
            android:exported="false"
            android:stopWithTask="true">
    </service>
```

OpenDDS service classes should extend `Service` and provide an `IBinder` for
an application to use when it creates the `ServiceConnection`. For example,

```Java
public class MainActivity extends AppCompatActivity {
    // ...
    private OpenDdsService svc = null;

    private ServiceConnection ddsServiceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            OpenDdsService.OpenDdsBinder binder = (OpenDdsService.OpenDdsBinder) service ;
            svc = binder.getService();
            // ...
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            // ...
        }
    }
    // ...
}
```

```Java
public class OpenDdsService extends Service {
    // ...
    private final IBinder binder = new OpenDdsBinder();

    public class OpenDdsBinder extends Binder {
        OpenDdsService getService() {
            return OpenDdsService.this;
        }

        @Override
        public void onCreate() {
            // ...
        }

        @Override
        public int onStartCommand(Intent intent, int flags, int startId) {
            return START_NOT_STICKY;
        }

        @Override
        public IBinder onBind(Intent intent) {
            return binder;
        }

        @Override
        public void onRebind(Intent intent) {
            super.onRebind(intent);
        }

        @Override
        public void onDestroy() {
            super.onDestroy();
            stopSelf();
        }
    }
}
```

See Android's [Services overview](https://developer.android.com/guide/components/services)
for more information.

## Footnotes

<a id="footnote-1"/>

1. The choice to support NEON or not is beyond the scope of this guide. See
   ["NEON Support"](https://developer.android.com/ndk/guides/cpu-arm-neon) for
   more information.
