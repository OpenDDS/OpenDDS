# OpenDDS Releases

## Version 3.14 of OpenDDS
OpenDDS 3.14 is currently in development, so this list might change.

### Additions:
- XTypes IDL annotations for topic types, Union topic types (#1067, #1422):
  - `#pragma DCPS_DATA_TYPE` and `#pragma DCPS_DATA_KEY` are now deprecated in
    favor of the `@topic` and `@key` IDL annotations.
  - Like structures, unions can now be used to define a topic.
  - See section 2.1.1 of the OpenDDS 3.14 Developer's Guide for more
    information on both of these features.
  - See `docs/migrating_to_topic_type_annotations.md` for a guide on how to
    migrate existing IDL to use topic type annotations.
- Added a C++11 mode to opendds_idl's code generation (#954, #1030, #1379)
- RtpsRelay, a server that assists with peer-based discovery on the internet, see the "Internet-Enabled RTPS" Chapter in the Developer's Guide (#1057, #1068, #1286, #1341)
- Support for IETF ICE for NAT firewall traversal, see the "Internet-Enabled RTPS" Chapter in the Developer's Guide (#1069)
- Dynamic network interface changes on supported platforms (#1365, #1551)
- An OpenDDS-specific Built-In Topic to report network location/connection details (#1389)
- Performance improvements to RTPS Discovery and the RTPS_UDP transport (#1045, #1177, #1138, #1202, #1251, #1260, #1262, #1265, #1324, #1402)
- CMake package for building applications that use OpenDDS, see `docs/cmake.md` (#981)
- Query Condition and Content-Filtered Topic can filter dispose/unregister messages (#900)
- Support for publishing using the [Node.js module](https://www.npmjs.com/package/opendds) with V8 (#1065)
- opendds_idl can generate code to convert IDL structures to RapidJSON (#1097, #1254)
- Full-message protection (RTPS Protection) in DDS Security (#1280)
- RTPS protocol version 2.4 (#1367, #1374, #1364, #1359, #1404, #1381, #1518, #1522, #1530, #1540)
- TCP transport: configurable connection timeout (#1493)
- A new benchmarking and performance analysis tool in `performance-tests/bench_2` (#1546)

### Platform Support and Dependencies:
- Support for Java Development Kit up to version 12 (#1137)
- Monitor GUI and the ishapes example now use Qt5, see `docs/qt.md` (#929, #932, #1083)
- Improved Android support, see `docs/android.md` (#1066, #1446)
- iOS support (#1535, #1545, #1551)
- Updated dissector to work with Wireshark 3 (#1080)

### Fixes:
- DDS Security improvements (#1310, #1482, #1531)
- Java API can now be used on Android (#1156)
- Support equals() and hashCode() in Java (#1331)
- Improved Java bindings builds (#1146, #1157, #1160, #1167)
- InconsistentTopicStatus support in RTPS Discovery (#1158, #1164)
- Presentation QoS coherent_access (#1094)
- RTPS_UDP transport supports combination of best effort and reliable readers of the same writer (#1449)
- Improved support for fragmented data (#1474, #1547)
- Topics need to be enabled to use with DataReader/Writer (#1193)
- TCP Reconnect (#1273, #1464, #1487, #1497, #1501, #1506, #1519, #1523, #1526)
- Shared memory transport improvements (#1301)
- GuardCondition _narrow (#1407)
- "make install" with DESTDIR (#1429, #1440)
- Support for multiple transport instances in the same transport configuration (#1465)

### Notes:
- The configure script now defaults to not including tests (#1287)
- Removed obsolete `-Gws` option in `opendds_idl`. This option produced sample
  descriptions for the Wireshark dissector before OpenDDS v3.7. (#966, #1031)

## Version 3.13.3 of OpenDDS
Released Fri Oct 11 16:13:35 UTC 2019

### Fixes:
- Updated IDL V8 generation to use decimal strings (#1055)
- Updated Dissector for Wireshark 3.0 (#1080)
- Added support for Visual Studio 2019 (#1053)
- .NET compatibility (#1096)
- Security: improved support for DH keys (#1092)
- Security: general updates (#1091, #1101, #1118)
- GoogleTest version update (#1112, #1115)
- Improved performance of GUID comparison and lookup (#1128, #1133)
- Fixed Monitor library interaction with DomainParticipant (#1132)
- Configure script MSVC version detection (#1141)
- Security with some submessages encoded and some plain (#1166)
- Receive Strategy buffer management (#1192)
- Error check in TcpTransport::configure_i (#1209)
- Service Participant, Reactors, and signals (#1206, #1215, #1231)
- Modeling SDK Eclipse plugins updated to latest Eclipse (#1222)
- Secure discovery for unregister/dispose (#1225, #1227, #1232)
- InstanceState and reactor interceptor with reader data lifecycle (#1237, #1248)
- Serializer: input validity check when aligning (#1239)
- Transport framework improvements (#1249)
- Use monotonic time for relative timeouts on platforms that support it (#1272)
- SPDP: start counter at a random value and detect bad sequence numbers (#1274)

## Version 3.13.2 of OpenDDS

### Fixes:
- Service Participant, Transport Registry, RTPS libs updated to support certain Restart scenarios (#1081)
- Wrong entity_refs_ counter when deleting data reader in the TopicDescriptionImpl (#936)
- Add an include required for std::back_inserter (#965)
- DDS Security on VxWorks7 (#968)
- Fix unregister with nil handle (#1064)
- Install with java: Fix classpath for i2jrt.jar and i2jrt_compact.jar in .mpbs (#1079)

## Version 3.13.1 of OpenDDS

### Fixes:
- rtps_udp: enhanced generation of gaps for durable data (#1001)
- rtps_udp transport: improved handling of socket errors (#1002)
- Fixed a bug in contains_entity for DataReaders (#944)
- Fixed set_qos method for not enabled DataWriters and DataReaders (#955)
- Fixed requiring Conditions to be cleaned up in DataReaders (#958)
- Fixed a locking issue in transport with Security enabled (#933)
- Fixed total_count fields in on Offered/RequestedDeadlineMissed (#969)
- Security: Fixing uninitialized values reported by valgrind
- Support static builds with DDS Security enabled (#967)
- Fixing build when no-builtin-topic and security flags are both configured
- Configure: fixed detection of MSVC compiler versions
- Configure on Windows: allow Perl to be located in a directory with spaces
- Configure: follow redirects if using curl to download (#1025)
- Configure: Android support (#964)
- Configure: added for --host-tools option (part of #968)
- Make install awareness of user_macros.GNU and improved Java bindings support (#1022)

## Version 3.13 of OpenDDS

### Additions:
- Includes a beta implementation of the OMG DDS Security 1.1 specification
- Monitor library can be enabled with -DCPSMonitor 1 or ini file
- Updated Wireshark dissector now supports Wireshark version 2.x
- MultiTopic: use Topic's QoS when creating constituent data readers

### Fixes:
- Memory management improvements
- Command-line argument -DCPSRTISerialization takes a 0/1 parameter
- Further refined fix for RtpsUdpDataLink::remove_sample locking
- Improved Java bindings build process when using GNU Make
- RTPS interoperability fixes from testing at OMG conferences
- OpenDDS can be completely shut down and restarted within a process
- Improved code generation for IDL unions, including in Javascript/V8
- Fix for in-process messaging in FACE TSS (PR #819)
- Fix for ReadCondition leak in OpenDDS::FaceTSS::receive_message (PR #832)
- TCP reconnect enhancements
- Removed unused sequence number tracking in DataReaderImpl
- Fixed a memory leak in content filtering on the subscriber side
- rtps_udp transport: improved handling of socket errors
- RTPS discovery: avoid skipping sequence numbers in SEDP
- InfoRepo: improved persistence with Built-in Topics

### Notes:
- The contrib directory, which contained unmaintained code, was removed

## Version 3.12 of OpenDDS

### Fixes:
- RtpsUdpDataLink::remove_sample locking
- track latency only when needed for LatencyBudget QoS or statistics
- corrected counters for sample rejected/lost liveliness lost total_count_change
- get_key_value() corrected return value for errors
- setting DCPSBitTransportPort without DCPSBitTransportIPAddress had no effect
- writer side association wasn't removed after the the reader had unsubscribed
- memory leaks
- issue with unregistered instances when deleting DataWriter
- problem with multiple transport instances in a single transport config
- EntityFactory QoS didn't enable child objects when the factory was enabled

### Notes:
- configure supports ACE+TAO from DOCGroup repository as an option
- configure improvements for using external ACE+TAO, cross-compiling on Windows
- coverity fixes
- improved Bench performance test
- Docker Hub now has an OpenDDS docker image

## Version 3.11 of OpenDDS

### Additions:
- Support for Visual Studio 2017
- Content-filter expressions can now use 64-bit integer literals
- tcp transport has a new wait_for_acknowledgements implementation
- DataWriter's HISTORY QoS processing now allows replacing old samples with new ones without blocking

### Fixes:
- Improved robustness of DEADLINE QoS processing
- Uses of content-filter expressions are now checked for the correct number of parameters
- Fixed versioned namespace support
- Improved support for IPv6
- Improved robustness of rtps_udp transport, especially when used with static discovery
- Use of the constant PARTICIPANT_QOS_DEFAULT no longer initializes the domain participant factory
- Bug fixes in the OpenDDS Wireshark dissector (for sample dissection)

### Notes:
- The QoS XML Handler library is now decoupled from DDS4CCM, it requires xerces

## Version 3.10 of OpenDDS

### Additions:
- Added support for versioned namespaces
- Reworked TypeSupport implementation in order to reduce exposure of internal headers to user code
- Added `read/take_instance_w_condition` (not in DDS spec)
- Time Based Filter QoS: fixed GitHub issue #268, samples can now be held and delivered after the timeout
- C\++11 updates to the ishapes example: make use of C++11 `shared_ptr` and `to_string` instead of using boost
- When C++11 is enabled, use noexcept(false) where necessary
- Extended TypeSupport to allow unregistering of types from a domain participant (not in DDS spec)
- RtpsDiscovery: allow config to specify which interface's MAC addr is used in GUIDs
- Wireshark dissector updates: support for more IDL constructs in data sample dissection

### Fixes:
- Liveliness QoS: fixed incorrect "liveliness lost" for automatic liveliness with sporadic writers
- Fixed RTPS ParticipantMessageData entityIds used by automatic and by-participant liveliness
- Fixed make install problem when DESTDIR is specified
- Allow fallback to next transport implementation when active side fails to connect using the first one
- Updated `TypeSupport::get_type_name()` to match the DDS spec

### Stability Improvements:
- Fixed many issues included in recent Coverity Scan defect reports
- Fixed a few reference counting issues in internal classes

### Notes:
- This is the final version we will test with TAO 2.0a, please upgrade to a newer TAO (see README.md)

## Version 3.9 of OpenDDS

- Updated how DataWriter QoS is interpreted by internal data structures:
  - RESOURCE\_LIMITS.max\_samples\_per\_instance controls total samples that can be
    stored per instance, independent of HISTORY
  - HISTORY.depth controls number of samples (per instance) that will be made
    available to late-joining readers if DURABILITY is enabled
    - KEEP\_ALL history uses max\_samples\_per\_instance as its depth
  - WriteDataContainer is more eager to remove samples that are no longer needed
  - WriteDataContainer will not remove samples that are required to implement
    DURABILITY, hence if RELIABILITY is enabled this can block a write() just
    like other "no resource available" scenarios

- Fixed an RTPS interoperability issue with the new ParticipantMessageData
  messages (used for liveliness)

- Enhanced RTPS support for DURABILITY QoS enabling faster reader/writer
  associations when DURABILITY is in use (set to TRANSIENT\_LOCAL)

- RTPS Discovery has a new option "SpdpLocalAddress" which can be used to
  bind SPDP's socket to a particular network interface

- InfoRepo discovery will use TAO's BiDir GIOP feature to reduce number
  of sockets needed (disable with -DCPSBidirGIOP 0)

- Intermediate Type Language (itl), a helper library used by the OpenDDS
  wireshark dissector, is now included in the source tree in
  tools/IntermediateTypeLang.  itl depends on the 3rd-pary lib rapidjson.
  When running the configure script, use --rapidjson=<location>

- Corrected makefile generation for Java bindings with IDL bounded strings

- Fixed a bug where some header files would be missing from "make install"

- "make install" now works with Java bindings

- Configure script: now supports FreeBSD, Mac OS X El Capitan,
  and cross-compiles for Linux targets (Raspberry Pi and Intel Edison tested).
  The configure script attempts to find the JDK if invoked with --java.

- cross-compilation now works with Java bindings, including use of JRE compact1



## Version 3.8 of OpenDDS

- Improved support for Safety Profile

- OpenDDS multicast transport (reliable mode) now guarantees in-order delivery

- Added a "default address" configuration parameter that sets an overall default
  for binding to a specific network interface

- Fixed bugs in transport framework, udp transport, InfoRepo discovery, static
  discovery, and the Liveliness and Durability QoS policies


## Version 3.7 of OpenDDS

### Additions:
- Implements FACE Transport Services API v2.1, FACE safety base and
  FACE safety extended profiles (Future Airborne Capability Environment).
- Implements DDS 1.4 and RTPS 2.2.
- Support allocations from memory pool in safety profile builds.  Introduced
  standard library containers and strings, as well as TAO sequences that
  allocate from this pool.
- To support FACE TS, the IDL struct DDS::SampleInfo has a new field
  opendds\_reserved\_publication\_seq as an extension to the DDS specification.
- Updated opendds\_idl for FACE support, moving some of the code-generation
  from tao\_idl to opendds\_idl when targeting FACE TS.
- Support IDL fixed type inside of DCPS\_DATA\_TYPE (used with FACE TS).
- Type-specific DataWriterImpl and DataReaderImpl are now templates.  Code
  generation now introduces a typedef of these.
- Content filtering now supports modulus (MOD) operator as an extension.
- Added a static (configuration-based) discovery mechanism.
- RTPS Discovery is now supported when OpenDDS is built without built-in topics.
- RTPS Discovery has a new configuration parameter SedpLocalAddress which
  configures the rtps\_udp transport used internally by discovery in the same way
  that local\_address configures a user-created rtps\_udp transport instance.
- Support non-default liveliness QOS in rtps\_udp transport.
- Support wait\_for\_acknowledgements in rtps\_udp transport.
- Support command line option -DCPSRTISerialization when integrating with RTI
  DDS using RTPS, to account for a different serialization algorithm.
- Configure script handles some cross-compile builds and safety profile.
- Support for new Visual Studio compilers (vc11, vc12, and vc14).
- Support for clang builds on Mac OS X.
- Removed support for old GCC compilers (<= 3.3).
- opendds\_idl command-line option -Gitl is a replacement for -Gws.  Wireshark
  dissector now reads ITL files, rather than .ini files.

### Fixes:
- Fixed many tests for minimum profile builds.
- Enforced correct behavior of a reliable data writer when history QoS
  is set to KEEP\_LAST with depth=1.
- opendds\_idl can now be specified in user\_macros.GNU instead of always
  assuming it is in $DDS\_ROOT/bin (for -type gnuace builds).
- Fixed initial IPv6 support in windows builds.
- Fixed disassociate handling of local entities on loopback interface.
- Corrected wait\_for\_acknowedgements for non-reliable writers to return
  success immediately.
- Allow disassociating readers to finish processing messages from active writer
  before removing.
- Remove TcpDataLink from DataLinkSet when connect() fails.  Previously, the
  datalink would remain and would be seen as a fully-connected datalink by new
  associations.
- DCPSTransportDebugLevel 1 messages are now association-oriented.
- Fixed durable resend out-of-order handling for rtps\_udp transport.
- Fixed handling of acknowledgements during shutdown of rtps\_udp data link.
- RTPS Discovery now ignores 'RTPX' messages seen during RTI integration.
- Dissection of unions in wireshark dissector (requires ITL).
- Corrected reference counting scheme for Recorder and Replayer objects:
  the reference-counted handle types changed from \_rch to \_var.

### Notes:
- OCI no longer builds with vc71, vc8, and GCC 4.1.1.  If you require support
  of these older platforms contact sales@ociweb.com.
- The project repository is now at https://github.com/objectcomputing/OpenDDS


## Version 3.6 of OpenDDS

### Notes:
- The discovery framework of OpenDDS has been refactored to be more
  asynchronous.  This should make discovery faster and more reliable
  for both publishing and subscribing clients, regardless of the
  discovery mechanism used (RTPS or InfoRepo).

- DataWriters now release locks more quickly than before. This allows the
  transport to be available to more threads when compared to 3.5.1.

- Improved DDS specification compliance with regards to reliability,
  resource limits, and history qos settings.  Specifically, reliable
  data writers may block for max\_blocking\_time and potentially return
  DDS::RETCODE\_TIMEOUT in situations where they would have simply
  written the data in older version of OpenDDS.  The new behavior
  is required by the DDS specification, see section 2.2.2.4.2.11
  of the version 1.4 specification for details.  This change affects
  various operations of the data writer interface including the
  write and register operations.

### Additions:
- The wireshark dissector now supports wireshark versions 1.8.x, 1.10.x,
  1.12.x, and 1.99.1.

- The modeling framework now support Eclipse 4.4 (Luna) and GMF 3.1.x

- Added a public\_address option for the tcp transport.  This option
  allows a tcp transport instance to listen on one address (local\_address)
  while advertising another address (public\_address) to discovery.
  This feature was used to allow a proxy process between the subscriber
  and publisher in the TcpReconnect test, but may also be useful in
  other deployment scenarios, especially using OpenDDS through a firewall.

### Fixes:
- There were a number of concurrency and thread utilization improvements.
  These changes fixed a number of threading issues as well as improving
  the efficiency of publication.

- Fixed some issues with RTPS discovery.

- Fixed a number of issues related to the reconnect functionality of
  the tcp transport.

- Fixed some bugs in the OpenDDS IPv6 support.

- Eliminate duplicate samples received during durable resend.

- Fixed memory leak at shutdown time in modeling library.

### Tests:
- Many updates to make test results more complete and deterministic.

- Added a TcpReconnect test that exercised the ability of the tcp
  transport to seamlessly reconnect OpenDDS clients when the transport
  layer drops a connection.

- Added a WriteDataContainer unit test to verify data writer memory
  management compliance with the DDS specification.

- Added a PubScalability test that was used to test association and
  publications with large numbers of subscribers.

- Added a packet reassembly unit test.


## Version 3.5.1 of OpenDDS

### Notes:

  - Due to addition control messages in the 3.5.1 release, mixing 3.5.0
  and 3.5.1 releases should work properly, but the 3.5.0 data readers can emit
  the following error message when associating with a durable 3.5.1 data
  writer:
    ERROR: DataReaderImpl::data\_received unexpected message\_id = 11
    Upgrade all endpoints to 3.5.1 to remove this error message.

### Additions:

- Support later (>= 4.7) versions of GCC compilers, and later (>= 2.0a)
  builds of TAO. Configure script now downloads 2.2a by default.

- Added IDL compiler support for typedef of string fields within unions.
  Added Java IDL compiler support for struct forward declarations.

- Updated UML design diagrams to reflect current code base.

### Fixes:

- DCPSInfoRepo no longer informs a subscriber about a crashed or killed
  publisher.  This made associations of current publishers and subscribers
  delayed until timeout was reached.

- Fixed deadlock condition in send\_delayed\_notifications() resulting from
  multiple datawriters.

- Fixed deadlock in DataLinkSet::send\_control().

- Fixed deadlock in WaitSet::wait() and DataReaderImpl::data\_received().

- Fixed multicast transport with multiple participants in a single process.

- Fixed parsing of -ReassociateDelay argument by DCPSInfoRepo.

- Fixed crash of long-lived zero-copy sequence when it is destroyed and
  crash of zero-copy sequence accessing released instances.

- Fixed unprotected access to DataWriterImpl::reader\_info\_,  and
  TransportSendStrategy::pkt\_chain\_.

- Fixed multiple RTPS crashes when participant is being deleted.

- Fixed RTPS crash when a data reader is being deleted.

- Fixed problem for non-unique repo ids for multicast when multiple
  (non-federated) DCPSInfoRepos are used on the same network.

- Fixed crash caused by back-pressure in transport causing the most recent
  sample for an instance to be dropped.

- Fixed error in reconstruction logic for fragmentation of udp and multicast
  transport packets.

- Fixed PublisherImpl and SubscriberImpl to properly set a DataWriterImpl
  or DataReaderImpl entity's deleted flag during deletion to allow
  association checks to properly identify entities that are being deleted and
  not proceed in association.

- Fixed problem for persisted DCPSInfoRepo where persisted participants
  were never dissociating.

- Fixed activity improperly occuring when a partipant is being deleted
  that caused pure virtual function calls. In one case, waiting now takes
  place during deletion of a data writer until all control messages sent during
  unregistration of instances are account for. In another case fixed so
  TransportSendControlElement will delete itself before it reports to the listener
  that the message has been delivered/dropped.

### Tests:

- Many updates to make test results more deterministic.

- Added Reliability test.

- Added ManyToMany test to test associations with additional participants,
  publishers, subcribers, data readers and data writers.

- Added PersistentInfoRepo test.

- Restored thrasher medium and high test cases.

- Rewrite Priority test to make it verify proper QOS functionality.

- Rewrite LargeSample test to make it verify message contents are as expected.


## Version 3.5 of OpenDDS

- Updates to RTPS support resulting from both interoperability
  testing (March 2013 OMG meeting) and user feedback.

- Fixed a bug in the DataReader relating to the Deadline timer.

- Generated TypeSupportImpl classes now contain nested typedefs that
  facilitate programming with C++ templates.
  See tests/DCPS/ManyTopicTest for an example of usage.

- Added config options to bind RTPS-related multicast sockets to
  specific network interfaces.  See the ChangeLog for details.

- Fixed an opendds\_idl code generated bug when typedefs of basic
  types are used as fields of structs.

- Corrected a number of other bugs related to discovery and scaling.


## Version 3.4.1 of OpenDDS

- Added a new option to opendds\_idl, -Wb,v8, which generates type support
  for copying DCPS structs from C++ objects to JavaScript objects -- requires
  the V8 JavaScript engine.  See https://npmjs.org/package/opendds for OpenDDS
  integration with Node.js.

- Fixed a bug in serialization with misaligned reads.  It only impacts
  certain platforms with strict alignment requirements such as SPARC/SunCC.

- Clang 3.2 is now a supported compiler.

- Fixed a bug in the rtps\_udp transport, in certain cases an invalid Gap
  submessage was sent which can result in data samples not being received.


## Version 3.4 of OpenDDS

- Added new OpenDDS-specific APIs for sending and receiving untyped data,
  (does not require code generation from IDL data structures).  See the
  Recorder and Replayer classes and the test in tests/DCPS/RecorderReplayer.

- Implemented the ability to send fragmented data with the rtps\_udp transport.

- Fixed a bug in copy\_from\_topic\_qos(): the Ownership policy is now copied.

- The optimization of publisher-side content filtering now applies to durable
  data in addition to newly written data samples.

- As with versions before 3.2, the DCPSInfoRepo object reference is allowed to
  fail to resolve.  If it fails, the current operation can be re-tried at a
  later time when the DCPSInfoRepo server is available.

- Added a new option for per-domain default transport configurations.

- Corrected IDL string constants for QoS policy names which were wrong in the
  DDS spec (GROUPDATA and TRANSPORTPRIORITY ...\_QOS\_POLICY\_NAME).  These are
  not used anywhere in OpenDDS.


## Version 3.3 of OpenDDS

- If an instance of a transport can't be set up, an exception is thrown so
  that different configuration parameters can be attempted by the user.

- Corrected the mapping of Priority QoS to DiffServ CodePoint DS subfield.

- For thread-per-connection send, if no explicit scheduling policy has been
  configured, OpenDDS will now inherit the current thread's policy/priority.

- Fixed a possible deadlock of RTPS discovery on Windows.

- Added non-blocking I/O setting to tcp sockets, VxWorks 6.4 required it.

- Fixed various bugs in deadline and ownership QoS.

- Improved multicast transport's robustness during handshaking when many
  connections are set up at the same time, and also allow for multicast
  to skip over received datagrams that don't have the expected header values.

- Multicast transport configuration can now specify a local network interface
  which will be used to join the multicast group.

- The support for the generation of the DDS4CCM needed LEM library has been
  moved to CIAO DDS4CCM. This removed the optional dependency on CIAO.


## Version 3.2 of OpenDDS

- Added a new transport type: Shared Memory
  Use shmem as the short name (in place of "tcp", "udp", "multicast", etc.).
  See dds/DCPS/transport/shmem/ShmemInst.h for configurable parameters.

- The udp transport now supports setting IP DSCP (DiffServ CodePoint) based on
  the writer's TRANSPORT\_PRIORITY QoS value.

- Fixed bugs in udp and multicast transports, including reassembly of fragments.

- Made several changes in order to support CIAO 1.1.2 with DDS4CCM. From
  this version DDS4CCM has support for OpenDDS as DDS vendor. Check the CIAO
  release notes for details on what has been ported.

- Added new QOS\_XML library with which QoS settings can be done through an XML
  file. Used at this moment by the CIAO DDS4CCM implementation.

- Implemented rejection of samples due to exceeding max\_instances and
  max\_samples.

- Made the RTPS discovery and transport code more robust.

- Refactored InfoRepo-based discovery into its own library. This reduces the
  dependency on TAO such that certain TAO libraries will no longer be loaded
  unless they are needed for InfoRepo-based discovery.

- For IDL files that only contain local interfaces, the generation of
  server-side TAO code was suppressed. The suppression applies to DdsDcps*.idl.
  This means, for example, that code that was including DdsDcpsSubscriptionS.h
  needs to be changed to include DdsDcpsSubscriptionC.h.

- To reduce library size, OpenDDS can now be conditionally compiled to exclude
  profiles discussed in the Compliance section of the DDS spec. See
  section 1.3.3 in the Developer's Guide for more information.


## Version 3.1 of OpenDDS

- This release includes the initial implementation of DDS Interoperability
  in OpenDDS, using the DDS-RTPS spec v2.1 from the OMG (formal/2010-11-01).

  RTPS support in OpenDDS consists of two main features:

    - A pluggable transport implementation, known as "rtps\_udp" because it
      implements the UDP PSM of the DDS-RTPS spec.  This is a peer of the
      existing OpenDDS-native transports: "tcp", "udp", and "multicast".

    - A new discovery mechanism for RTPS's distributed discovery.  This can
      replace the DCPSInfoRepo in existing OpenDDS deployments.

  Neither of these features is enabled by default.  See the OpenDDS Developer's
  Guide chapter 7 for configuration details.  Also, note that not every OpenDDS
  feature (for example, certain QoS values) is supported with RTPS in the
  initial implementation.  These exceptions are documented in the Developer's
  Guide sections 7.3.3 and 7.4.5.5.

- Changed set\_qos() of DataWriter, DataReader, Publisher and Subscriber so
  when provided a QoS value incompatible with an existing association, that the
  value will be changed, association will be broken, and RETCODE\_OK returned.
  Previously, if any association would be broken, no change would take place
  and false would be returned.  New version is compatible with DDS spec.

- OpenDDS udp transport enhancements:
    - Added send\_buffer\_size and rcv\_buffer\_size parameters to the udp
      transport configuration.
    - The default local address will use DNS host names instead of IP
      addresses.
    - Added support for IPv6 when ACE is built with IPv6 support.

- OpenDDS multicast transport enhancements:
    - Added a configuration parameter "async\_send" (defaults to false)
      that will send multicast datagrams using async I/O if supported
      by the platform.  As of now this is only implemented on Windows.
      It could be extended to Unix aio\_*() functions in theory, but these
      are generally not preferred and the regular socket functions are
      sufficiently fast.

- Removed DataReaderQosExt from DdsDcpsSubscriptionExt.idl.  This was only
  being used to provide a non-standard-compliant backwards-compatibility
  setting to get the DURABILITY QoS behavior matching that of OpenDDS v1.2
  and earlier.  It was not being used in any OpenDDS code.


## Version 3.0.1 of OpenDDS

- The DCPSInfoRepo reassociates Built in Topic connections for existing
  datareaders and datawriters when restarted from persistence.

- The opendds\_idl compiler will now print a warning about not including TAO's
  orb.idl file when a file named orb.idl is included.

- Fixed a bug where the timeout for passive\_connect\_duration was ignored.

- Fixed multiple bugs in fragmentation/reassembly used for udp and multicast.

- Fixed multicast loopback and session re-use problems.

- Fixed a bug with memory management for the TransportCustomizedElement
  used in publisher-side content filtering.

- New example Messenger.minimal - which is like messenger, but streamlined
  for new users to understand the whole easier.


## Version 3.0 of OpenDDS

- Transport configuration has undergone a major redesign and any existing
  transport application code or transport configuration files need to be
  updated.  See the Developer's Guide for details of the design.  See
  $DDS\_ROOT/docs/OpenDDS\_3.0\_Transition.txt for a description on how
  to migrate your applications to the OpenDDS 3.0 design.  As a part of
  this design OpenDDS gains the following capabilities:
    - Most applications can do all transport configuration via
      configuration files (no transport source code required).
    - Individual writers and readers can now support multiple
      transports.

- The simpleTcp transport was renamed to tcp (the directory containing
  its source code was also renamed).

- The udp transport no longer required a local\_address when being
  configured.  If not specified, the property defaults to a value
  selected by the operating system (similar to the tcp transport).

- Details of the OpenDDS Modeling SDK changes in this release are documented
  in the Eclipse-based help, see OpenDDS Modeling SDK Guide -> Getting
  Started -> Introduction to the OpenDDS Modeling SDK.

- First release of OpenDDS Real-Time Data (RTD) for Excel an Excel Add-in
  and RTD Server.  It allows visualization of an operating OpenDDS system
  from within Excel. It gives a tree view of a repository that mirrors the
  stand-alone OpenDDS Monitor application's tree view, but with the
  additional capabilities to automatically restart monitoring of a repository
  when a saved workbook is opened, monitoring multiple repositories
  simultaneously, and snapshot a repository monitor tree view.

- Wireshark dissector improvements include removal of the restriction
  that inforepo IOR be placed in a special file.  Dissector config
  files can now be generated by opendds\_idl.

- The monitor tool has a new view which is an alternate to the Graphviz
  view.  The Node view uses Qt gui elements to model the OpenDDS system.
  Users can manipulate Node view graphs before saving them to PNG files.

## NEWS for Previous Major Versions

- NEWS for OpenDDS release 2.X versions are archived in [docs/history/NEWS-2.md](docs/history/NEWS-2.md)
- NEWS for OpenDDS release 1.X versions are archived in [docs/history/NEWS-1.md](docs/history/NEWS-1.md)
- NEWS for OpenDDS release 0.X versions are archived in [docs/history/NEWS-0.md](docs/history/NEWS-0.md)
