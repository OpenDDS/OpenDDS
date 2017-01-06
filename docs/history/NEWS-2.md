Version 2.4 of OpenDDS.
-------------------------------------------------------------------------------

- This will be the final release of OpenDDS that will build with the current
  patch level of TAO 1.5a.  This release is compatible with (the current patch
  levels of) TAO 1.5a, and 1.6a, as well as the current DOC Group release.
  Future releases will be compatible with TAO 1.6a and the DOC group -- see
  the README file for details on TAO versions.

- The implementation of ContentFilteredTopics now defaults to evaluating
  the filter expression at the publisher so that data samples which would
  be ignored by the subscriber can be dropped before getting to the transport.
  This feature can be turned off with "-DCPSPublisherContentFilter 0" or the
  equivalent setting in the [common] section of the configuration file.
  When using non-default DEADLINE or LIVELINESS QoS policy values, special
  consideration needs to be given to how these "missing" samples impact the
  QoS behavior, see the document in docs/design/CONTENT\_SUBSCRIPTION.

- DisjointSequence, an internal class used to track sequence numbers for both
  multicast and wait\_for\_acks, was enhanced to store contiguous ranges instead
  of individual values.  Thus it will not require so much memory in cases where
  there are gaps in the sequence numbers.

- Began implementing the DDS Interoperabiltiy specification aka "RTPS"
  (see OMG formal/2010-11-01).  Changes in this release include fragmentation
  and reassembly for udp and multicast, 64-bit sequence numbers, and the ability
  to transmit only the key fields for the instance-control messages (register,
  unregister, dispose).

- The monitor tool, an instrumentation viewer application for OpenDDS,
  has new options for generating and viewing directed graphs.  The tool
  generates Graphviz-readable .dot files, manages converting the files to
  representative .png files, and displays the generated .png files in its
  GUI.  Monitor executes Graphviz command-line tools in a separate process.
  There are no compile-time or run-time dependencies on Graphviz.  The
  monitor tool will run on systems without Graphviz installed but will
  not be able to generate .png files.

- Enhanced the Wireshark packet dissector to inspect SAMPLE\_DATA message
  contents. This requires a data model expressed in an .ini file (documented
  in tools/dissector/README).  In order to associate a SAMPLE\_DATA message
  with a type, the publication ID is harvested from InfoRepo GIOP messages.
  In order to detect InfoRepo GIOP messages, the IOR for the InfoRepo must
  be written in a file called "IOR.txt" in the current working directory when
  wireshark is invoked.

- Enhancements and fixes of OpenDDS Modeling SDK.  Version: 1.1.0.
    * Topic types from a DcpsLib shown in Project Explorer can be
      dragged to diagram for another DcpsLib. This allows the same
      Topic to be used in multiple DcpsLibs.
    * For more, see the Eclipse online help, under "OpenDDS Modeling
      SDK Guide > Getting Started"

- Enhancements and fixes of OpenDDS Modeling SDK.  Version: 1.0.1.
    * For more, see the Eclipse online help, under "OpenDDS Modeling
      SDK Guide > Getting Started"

_______________________________________________________________________________

Version 2.3 of OpenDDS.
-------------------------------------------------------------------------------

- First release of the OpenDDS Modeling SDK, a modeling tool that can be
  used by the application developer to define the required middleware
  components and data structures as a UML model and then generate the
  code to implement the model using OpenDDS.  The generated code can
  then be compiled and linked with the application to provide seamless
  middleware support to the application.  UML models are manipulated using
  a graphical editor based on Eclipse.  See the OpenDDS Developer's Guide
  <http://download.ociweb.com/OpenDDS/OpenDDS-latest.pdf> for
  installation instructions.

- DCPSInfoRepo no longer requires an -ORBSvcConf argument when using
  Built-In Topics.  The DCPSInfoRepo process will take care of loading
  SimpleTCP (if it's not already loaded).

- Fixed method signature of DataWriter::register\_instance\_w\_timestamp()
  to have two arguments per the latest spec, not three.

- "make install" is now available on platforms using GNU Make and when
  building with OCI TAO 1.6a or DOC Group TAO.

- Added support for Microsoft Visual Studio 2010 (vc10).

_______________________________________________________________________________

Version 2.2 of OpenDDS.
-------------------------------------------------------------------------------

- Completed the implementation of the Content-Subscription Profile (which
  was begun in version 1.3 of OpenDDS).  This includes:
  - QueryCondition (previously OpenDDS had a partial implementation)
  - ContentFilteredTopic
  - MultiTopic
  The content-subscription profile can be disabled at compile-time in order
  to reduce footprint: set the MPC feature "content\_subscription" to 0.

- The code generation tools for OpenDDS have changed in this release.  The
  Perl script "dcps\_ts.pl" is no longer used and instead the OpenDDS IDL
  Compiler "opendds\_idl" takes its place.  opendds\_idl also includes the
  functionality which used to be in tao\_idl and enabled with -Gdcps.
  Existing applications that use MPC do not need to make any changes unless
  they happen to depend on some rarely-used arguments to dcps\_ts.pl.  See the
  OpenDDS Developer's Guide for the list of arguments accepted by opendds\_idl.

- Renamed odds\_repoctl command to repoctl.  Add a decorator if needed when
  installing external to the OpenDDS directories.

- Renamed odds\_monitor command to monitor.  Add a decorator if needed when
  installing external to the OpenDDS directories.

- Renamed Wireshark dissector 'odds' library to 'OpenDDS\_Dissector'.
  The source directory was renamed from $DDS\_ROOT/tools/odds\_dissector
  to $DDS\_ROOT/tools/dissector.

- Fixed transport so it now allows associations between datareaders and
  datawriters attached to the same transport to work.

- Fixed a bug that causes samples larger than 64K to not work correctly with
  the SimpleTCP transport.

- Added nak\_delay\_intervals and nak\_max configuration options for reliable
  multicast to help reduce naks.

- Added support for Ownership qos.

- Added support for GROUP presentation.

- Changed the default DataWriterQos reliability max\_blocking\_time from infinite
  to be 100ms to be consistent with spec 1.2.

- A new "configure" script is available to assist with setting up prerequisites
  for OpenDDS compilation, including optionally downloading ACE+TAO.  See the
  INSTALL file for details.

_______________________________________________________________________________

Version 2.1.3 of OpenDDS.
-------------------------------------------------------------------------------

- Various fixes for memory management issue on transport framework.

- Fixed duplicate message issue caused by a lost link.

- Memory leak fixes.

- Fixed deadlocks on reliable multicast transport during association
  and shutdown periods.

- Improved reliable multicast transport and reduced nak overflow.

- Extended the capabilities of the OpenDDS-Bench performance test
  framework.  This consists mostly of expanded pre-configured tests as
  well as additional execution and plotting scripts.  The user guide at
  performance-tests/Bench/doc/user\_guide.html contains the details.

- Masked interrupts in threads created by OpenDDS to avoid failures
  while in system calls.
  Example: ERROR: ACE::handle\_write\_ready return -1 while waiting
                  to unclog. handle\_write\_ready: Interrupted system call

- Added support to set the TTL for multicast transports.

_______________________________________________________________________________

Version 2.1.2 of OpenDDS.
-------------------------------------------------------------------------------

- **This is a bugfix release to account for issues introduced by the
  previous version.**

##### New to this version are the following changes:

- Various bugfixes for locking issues during sample removal.

- Fixed a bug in obtain\_buffer() that did not return the TIMEOUT error
  code when max\_blocking\_time is 0 or a very small period.

- Fixed a deadlock in the new IP multicast transport caused when data
  is received while a reliable session is handshaking.

- Fixed an issue which caused spurious synch threads to be created for
  connectionless transports (i.e. UDP/IP, IP multicast).

- Enhanced the experimental UDP/IP transport to support multiplexing
  samples to unique endpoints rather than unique entities.

_______________________________________________________________________________

Version 2.1.1 of OpenDDS.
-------------------------------------------------------------------------------

- **This is a bugfix release to account for issues introduced by the
  previous version.**

##### New to this version are the following changes:

- Various bugfixes have been applied to the new IP multicast transport
  and the Extensible Transport Framework (ETF) to improve stability.

- Added IDL for ContentFilteredTopic and MultiTopic, but they are not yet
  implemented (operations will return NULL or RETCODE\_UNSUPPORTED).

- A Wireshark 1.2.x compatible dissector is included in this release.
  See $DDS\_ROOT/tools/odds\_dissector/README for details.

_______________________________________________________________________________

Version 2.1 of OpenDDS.
-------------------------------------------------------------------------------

##### New to this version are the following changes:

- The OpenDDS-Bench performance testing framework has been enhanced to
  include the following:
    - extended to allow fixed rate testing;
    - added predefined latency test configuration files;
    - added data reduction scripts to post process test results;
    - added data visualization scripts using gnuplot to create charts
      from reduced test data.
  Documentation is located at: $DDS\_ROOT/performance-tests/Bench/doc.

- The monitor library introduces a new instrumentation mechanism for
  reporting OpenDDS run-time information.  This information is
  intended to supplement that provided by the standard-defined
  Built-In-Topics.  See $DDS\_ROOT/dds/monitor/README for further
  details.

- A new instrumentation application is available to view executing
  OpenDDS service Entities.  This is located at: $DDS\_ROOT/tools/odds\_monitor.

- The opendds\_repo\_ctl tool has been moved from $DDS\_ROOT/dds/tools to
  $DDS\_ROOT/tools.  The name has also been changed to odds\_repoctl to
  reflect changes in naming conventions in this release.

- A new multicast transport is available which replaces the older
  ReliableMulticast and SimpleMcast transports.  For a detailed description
  of the design, configuration, known issues, and potential enhancements,
  see: $DDS\_ROOT/docs/html/multicast/multicast.html.

- A new udp transport is available which replaces the older SimpleUdp
  transport.  Like SimpleUdp, udp provides best-effort reliablity only;
  its use should be considered experimental.

- Numerous bugs in the Extensible Transport Framework (ETF) have been
  addressed to improve the stability of new and existing transports.

- Formal ranges of DDS::DomainId\_t values are now specified; users have
  full access to all DomainId values between 0x0 and 0x7fffffff.  All other
  values are reserved for use by the implementation.

_______________________________________________________________________________

Version 2.0.1 of OpenDDS.
-------------------------------------------------------------------------------

- **This is a bugfix release to account for issues introduced by the 2.0
  integration process.  No new features have been added in this release.**

##### New to this version are the following changes:

- Bugfixes for STLport support using Sun CC compilers.

- Corrected default ReliablityQoS policy for DataWriter entities.

- Bugfixes to Java language bindings including TAO\_IDL interface changes
  introduced in DOC TAO 1.7.4 and incorrectly generated project files.

_______________________________________________________________________________

Version 2.0 of OpenDDS.
-------------------------------------------------------------------------------

- **This is the first release of OpenDDS that conforms to the minimum
  compliance profile as documented in formal/07-01-01 (version 1.2).
  Near-complete support for the persistence profile and initial support
  for the content-subscription profile is also provided in this release.**

##### New to this version are the following changes:

- Updated support for DataWriter::unregister() and DataWriter::dispose()
  to properly release resources after all samples in a marked instance
  are removed.

- Implemented Publisher::wait\_for\_acknowledgments() as defined in
  formal/07-01-01.

- Fixed an issue in QueryCondition where applying filters on non-valid
  data samples caused a SEGV; these samples are now implicitly filtered
  out.

- Implemented PRESENTATION QoS policy as defined in formal/07-01-01.
  Currently, GROUP access is not supported.

- Implemented MANUAL\_BY\_TOPIC and MANUAL\_BY\_PARTICIPANT LIVELINESS QoS
  policy kinds.

- Implemented BY\_SOURCE\_TIMESTAMP DESTINATION\_ORDER QoS policy kind.
  Samples may now be ordered based on reception or source times on a per
  instance basis.

- Implemented ENTITY\_FACTORY auto\_enable support in formal/07-01-01.
  Entities may now be enabled manually by setting this value to false.

- Implemented WRITER\_DATA\_LIFECYCLE as defined in formal/07-01-01.
  Instances will now be properly unregistered/disposed when a DataWriter
  is deleted.

- Implemented READER\_DATA\_LIFECYCLE as defined in formal/07-01-01.

- Implemented TIME\_BASED\_FILTER as defined in formal/07-01-01.

- create\_*() functions now take a StatusMask argument.  A new constant
  named OpenDDS::DCPS::DEFAULT\_STATUS\_MASK has been introduced as a
  convenience to indicate interest in all status changes.

- Implemented the following new methods on DomainParticipant as defined
  in formal/07-01-01:
    - get\_discovered\_participants()
    - get\_discovered\_participant\_data()
    - get\_discovered\_topics()
    - get\_discovered\_topic\_data()

- Constant name changes:
    - PUBLICATION\_MATCH\_STATUS  => PUBLICATION\_MATCHED\_STATUS
    - DURATION\_INFINITY\_SEC     => DURATION\_INFINITE\_SEC
    - DURATION\_INFINITY\_NSEC    => DURATION\_INFINITE\_NSEC
    - TIMESTAMP\_INVALID\_SEC     => TIME\_INVALID\_SEC
    - TIMESTAMP\_INVALID\_NSEC    => TIME\_INVALID\_NSEC

- Removed constants:
    - TRANSPORTPRIORITY\_QOS\_POLICY\_NAME

- Type name changes:
    - StatusKindMask            => StatusMask
    - SubscriptionMatchStatus   => SubscriptionMatchedStatus
    - PublicationMatchStatus    => PublicationMatchedStatus

- Removed types:
    - SampleStateSeq
    - ViewStateSeq
    - OctetSeq

- Other type changes:
    - BuiltinTopicKey\_t is now a struct containing an array rather
      than a bare array type.  This allows passing a BuiltinTopicKey\_t
      by reference.
    - Ownership is now indicated in DataReaderQos and DataWriterQos as
      well as PublicationBuiltinTopicData and SubscriptionBuiltinTopicData.
    - Removed sevice\_cleanup\_delay from DurabilityQosPolicy.

- Method name changes:
    - register()/\_cxx\_register()  => register\_instance()
    - register\_w\_timestamp()      => register\_instance\_w\_timestamp()
    - unregister()                => unregister\_instance()
    - unregister\_w\_timestamp()    => unregister\_instance\_w\_timestamp()
    - on\_subscription\_match()     => on\_subscription\_matched()
    - on\_publication\_match()      => on\_publication\_matched()

- Other method changes:
    - set\_qos(), get\_qos(), get\_*\_status() and assert\_liveliness() now
      return error codes as specified in formal/07-01-01.
    - Subscriber::get\_datareaders() reader parameter has changed from out
      to inout.

_______________________________________________________________________________
