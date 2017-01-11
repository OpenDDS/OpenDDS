Version 1.3 of OpenDDS.
-------------------------------------------------------------------------------

- **This will be the final release of OpenDDS that will build with the current
  patch level of TAO 1.4a.  This release is compatible with (the current patch
  levels of) TAO 1.4a, 1.5a, and 1.6a, as well as the current DOC Group
  beta/micro release.  Future releases will be compatible with TAO 1.5a, 1.6a
  and the DOC group -- see the README file for details on TAO versions.**

##### New to this version are the following changes:

- Implemented the Communication Status entity structures and behaviors to
  conform with the version 1.2 specification (formal/07-01-01).  This
  includes member name changes and additions to LivelinessChangedStatus;
  modifying and extending the SampleRejectedStatusKind enumeration; and,
  adding new members to the PublicationMatchStatus and
  SubscriptionMatchStatus structures.

- Implemented the TRANSPORT\_PRIORITY Quality of service policy using the
  policy value to set thread priorities and DiffServ codepoint values.
  The current implementation does not honor the Changeable behavior of
  the policy.  Only the intially established policy value is honored for
  any given DataWriter Entity.  Thread priorities are not currently set
  on Windows systems.

- Implemented the LATENCY\_BUDGET Quality of service policy as a
  monitoring feature with an extended listener callback for exceeded
  budget values.  Statistics for latency are made available via an
  OpenDDS-specific extended API as well.

- Implemented the ReadCondition interface and associated operations
  on the DataReader (such as read\_w\_condition).

- Implemented the QueryCondition interface, but only queries of the form
  "ORDER BY a[, b, c...]" have any effect.  In other words, it's a mechanism
  that controls the order of the data-set received from any given read or
  take operation.  The ORDER BY fields can be either primitive types
  (including strings) or primitive types in nested structs (using a.b).

- Corrected a bug in the LIFESPAN QoS implementation where the Topic QoS
  was used.  Now the DataWriter QoS takes effect.

- The PARTITION QoS policy now accepts character class wildcards using [].

- Corrected a bug in the PARTITION QoS implementation where the default
  QoS would not match against the '*' wildcard.

- The DURABILITY QoS implementation was corrected so that readers requesting
  the VOLATLIE kind will not receive historic samples from writers.  Since
  there were some use-cases for the old behavior, it is available through an
  OpenDDS-specific interface -- see $DDS\_ROOT/dds/DdsDcpsSubscriptionExt.idl.

- When the writer's DURABILITY QoS is PERSISTENT, the format of the
  data stored on the filesystem has changed.  Instead of a single file in the
  current working directory, a directory is now used.  This is configurable
  using the option -DCPSPersistentDataDir.

- A Java Message Service 1.1 (JMS) provider built on top of OpenDDS is now
  available in $DDS\_ROOT/java/jms.  JMS Topics are supported (not queuing).
  See the README in that directory for more info.

- The $DDS\_ROOT/bin/federation utility has been renamed to opendds\_repo\_ctl
  and now supports shutting down a remote non-federated DCPSInfoRepo process.

- Deleting a DataWriter from a Publisher now blocks until pending samples have
  been delivered by the underlying transport.  By default, the DataWriter will
  block indefinitely.  To change the default blocking behavior, you may
  specify a number of seconds to block with the -DCPSPendingTimeout option.

- Instance handles are now compliant with version 1.2 of the DDS
  specification.  Additionally, the contains\_entity and get\_instance\_handle
  operations have been implemented.

- Implemented DataWriter::wait\_for\_acknowledgments() from formal/07-01-01.

- Added support for Windows Mobile 6 (WinCE) Standard SDK, and ACE\_USES\_WCHAR
  builds in general.  See docs/design/wchar.txt.

_______________________________________________________________________________

Version 1.2 of OpenDDS.
-------------------------------------------------------------------------------

##### New to this version are the following changes:

- Implemented the WaitSet, GuardCondition, and StatusCondition interfaces
  from the OMG DDS specification.

- Implemented DomainParticipant::get\_current\_time() from formal/07-01-01.

- Added the ability for using multiple repositories to provide redundant
  service metadata management.

  The ability to federate repositories and redirect applications to
  attach to a new repository in the event a repository fails or becomes
  unreachable.  This is an experimental feature.

- Removed requirement for repository domain specification files.

  Repositories will manage DomainParticipants and their contained
  Entities in any domain without requiring the domain to be prespecified
  in a configuration file.  This removes the '-d' command line argument
  from the repositories as well.

- Internal service Entity identifiers have been expanded to 128 bits.

  These internal identifiers were previously 32 bits.  The 128 format
  follows the format of OMG Specification formal/08-04-09 (The Real-time
  Publish-Subscribe Wire Protocol DDS Interoperability Wire Protocol
  Specification, Version 2.0) GUID\_t unique identifier type.

  OpenDDS places the identifier value for the DataWriter Entity of a
  publication on the wire to identify the source of data samples.  This
  means that this version of OpenDDS WILL NOT INTEROPERATE with previous
  versions since the 128 values are not compatible with the previous 32
  bit values.

- Java Bindings for OpenDDS

  See $DDS\_ROOT/java/README, $DDS\_ROOT/java/INSTALL, $DDS\_ROOT/java/FAQ

- dcps\_ts.pl changes

  Modified this script to create a single idl, header, and source file
  no matter how many types are contained within the input idl file.  This
  allows the output file names to be based upon the input file name (instead
  of the types found in the idl file).  Users of prior OpenDDS versions can
  use the script $DDS\_ROOT/bin/update\_dcps.pl to assist with this transition.

  Added a command line argument to allow an output directory to be
  specified.  All output files will be placed into this directory if
  present.  They will continue to be placed in the current directory if
  this argument is not present.

- DCPSInfoRepo changes

  A new library has been introduced (libDCPSInfoRepoServ) which provides
  the neccessary hooks for running the DCPSInfoRepo within another process.
  For example usage, see: $DDS\_ROOT/dds/InfoRepo/DCPSInfoRepo.cpp.
  
  
- Fixed bug in DCPSInfoRepo that did not re-evaluate the compatibility and
  associations when QoS change is made.

- Fixed bugs that affected connection establishment.

  Fixed DataWriterImpl to handle the case that remove\_association is called
  before fully\_associated is called.

  Fixed a race condition that connection delay deletion removes new assocaitions.

  Corrected fully association ack verification to fix issues when multiple
  associations of a datalink are added simultaneously.

- Added datalink\_release\_delay and removed keep\_link configuration.

  The datalink\_release\_delay is the delay for datalink release after no
  associations. It defaults to 10 seconds. This is to fix disconnect
  problem happened when removing and adding association related to
  same datalink during short period.

  Removed keep\_link which can be replaced with a big value of
  datalink\_release\_delay.

- Fixed a potential deadlock in reliable multicast transport.

_______________________________________________________________________________

Version 1.1 of OpenDDS.
-------------------------------------------------------------------------------

##### New to this version are the following changes:

- Added support for PARTITION QoS policy.

  This QoS policy allows publishers and subscribers to belong to one
  or more logical partitions.  Partition names may contain wildcards.
  However, only wildcards containing "*" or "?" are currently
  supported.

- Added support for DEADLINE QoS policy.

  The DEADLINE QoS policy allows the application to detect when data
  is not written or read within a specified amount of time.  The
  {Offered,Requested}DeadlineMissesStatus::last\_instance\_handle is
  currently not updated.

- Added support for LIFESPAN QoS policy.

  This policy allows the application to specify when data expires,
  meaning invalid.  Expired data will not be delivered to
  subscribers.

- Added support for the TRANSIENT kind of DURABILITY QoS policy.

  Data will outlive DataWriters when TRANSIENT DURABILITY is
  configured, and will be sent once a new DataWriter is created within
  the same domain for the same topic and type.  The cached data will
  only last as long as the process.

- Added support for the PERSISTENT kind of DURABILITY QoS policy.

  PERSISTENT DURABILITY provides basically the same functionality as
  TRANSIENT DURABILITY, except that cached data will survive process
  destruction.

- Added support for the DURABILITY\_SERVICE QoS policy.

  This QoS policy provides a way to specify resource limits for
  domain/topic/type-specific data in a TRANSIENT or PERSISTENT
  DURABILITY cache.  The resource limits in question are analogous to,
  although independent of, those found in the HISTORY and
  RESOURCE\_LIMITS policies.

- Added a new command-line option, "-DCPSTransportDebugLevel".

  This can be used to specify a debug level for the OpenDDS
  transport layer at run-time. OpenDDS debug statements range
  from 0 to 5 in increasing order of verbosity.

- Added the ability to attach to multiple DCPSInfoRepo processes.

  Applications which communicate using DDS are no longer restricted to
  communicating using domains controlled by a single DCPSInfoRepo
  process.  Individual domain publications and subscriptions within a
  process can be attached to a designated repository.  Each domain is
  controlled by a single repository and this association may not be
  changed for the lifetime of the repository or the application.

- Added QoS propagation.

  The client applications can change the qos of DCPS entity via
  set\_qos() call. The qos updates are propagated to DCPSInfoRepo which
  publishes the updated qos to corresponding BIT datawriter and also
  save it to persistent repository.

- Fixed a few bugs in persistent DCPSInfoRepo support.

  Fixed incorrect actor type and qos type for subscriber/datareader
  problem in UpdateManager::add(const UA& actor). This bug causes the
  reloading from persistent file maps the subscriber/datareader as
  publisher/datawriter.

  Fixed a bug in qos serialization. The TAO\_OutputCDR stream was not
  fully copied to buffer when the qos contains sequence data. The
  TAO\_OutputCDR stream needs be consolidated before memcpy.

- Fixed compliance with qos RxO.

  Fixed compliance with the DDS specification that qos parameters that
  Durability kind and Deadline.period be compatible between the
  subscriber and the publisher (RxO - Received x Offered)

- Improved Memory Mangament.

  Fixed issues with reference counting for classes derived from
  LocalObject.  If a user defined application is crashing on exit or
  cleanup, see the OpenDDS FAQ.

- Added support for IPv6.

  The requirement for using IPv6 is the fully qualified domain name (FQDN)
  needs be resolved as IPv6 address as it is used by default configuration
  to support BuiltInTopic.

  The local\_address in the configuration file should use IPv6 decimal
  address or hostname that can be resolved as IPv6 address.

- Used the address string instead of 4-byte IP address and port number
  for pub/sub endpoint address advertisement.

  The address string in the configuration can use decimal IPv4/IPv6 address
  or hostname that can be resolved on both pub and sub sides.

  The default address string is "host:port". The "host" is the fully qualified
  domain name and the "port" is picked randomly by OS.

- Fixed a problem that the default local\_address does not work when hostname
  is resolved as the loopback ip address.

  With the default local\_address, the acceptor now is listening on IPv4/IPv6
  ANY, but use the fully qualified hostname for address advertisement.

- Added default configuration for the reliable multicast transport type.

  The multicast\_group\_address defaults to the ACE default multicast address
  (224.9.9.2:20001 for IPv4 and ff05:0::ff01:1:20001 for IPv6) and the
  local\_address defaults to ANY with a random port picked by the OS.
  

- Fixed a bug in DataReaderImpl::remove\_associations() and
  DataWriterImpl::remove\_associations() that should not attempt to remove
  the already removed writer/reader.

- Fixed a bug in SimpleTCP that an active disconnect due to the peer
  graceful disconnect does not terminate send. This bug made the re-established
  connection fail on send strategy state check.


- Added dispose() and unregister() notifications.  When dispose()
  is explicitly called on the DataWriter side, the DataReader listener
  on\_data\_available() callback should receive a sample with the valid\_data
  flag == 0 and the instance state == NOT\_ALIVE\_DISPOSED\_INSTANCE\_STATE.

  When unregister() is explicitly called on the DataWriter side,
  the DataReader listener on\_data\_available() callback should receive a
  sample with the valid\_data flag == 0 and the instance state ==
  NOT\_ALIVE\_NO\_WRITERS\_INSTANCE\_STATE.


- Moved TRANSPORTTYPE\_QOS\_POLICY\_NAME and TRANSPORTTYPE\_QOS\_POLICY\_ID
  from DDS namespace to OpenDDS namespace since they are specific to
  OpenDDS.

_______________________________________________________________________________

Version 1.0 of OpenDDS.
-------------------------------------------------------------------------------

##### New to this version are the following changes:

- This product has been renamed to OpenDDS from "TAO DDS"

  Namespace and module names have been changed accordingly, except for a few
  cases where we are counting on certain names being generated by the TAO\_IDL
  compiler.


- The default participant setting for BITs is now "on"

  This change was actually part of 0.12 but the release notes were not updated
  to include it.  Please see the entry with the same title under the "Version
  0.12" section below for more details.


- Simplified .mpc and .mpb files

  Projects that use DDS no longer need to explicitly list "portableserver" as a
  base project.  It will be included by the "dcps*" base projects such as
  "dcpsexe" and "dcpsexe\_with\_tcp".  Also, an .mpb file is provided for each of
  the transport libraries (dcps\_tcp.mpb, dcps\_unreliable\_dgram.mpb,
  dcps\_reliable\_multicast.mpb).


- Supported built-in sequences (the CORBA::*Seq sequences).

##### Implementation details:
  - DDS keeps copies of the $TAO\_ROOT/tao/*Seq.pidl in $DDS\_ROOT/dds/CorbaSeq
    directory and renamed to *.idl files. These idl files are compiled with a 
    new IDL option -Gdcpsonly to just generate the DDS specific code for built 
    in sequences. The DDS CORBA::*Seq generated code will include the 
    TAO CORBA::*Seq generated code so both TAO and DDS functions will be available.


  - When a DDS application contains an idl file that includes
    $TAO\_ROOT/tao/*Seq*.pidl, the generated code will include the DDS
    specific built in sequences code(under $DDS\_ROOT/dds/ directory)
    instead of TAO specific built in sequences code(under
    $TAO\_ROOT/tao/ directory).

##### Notes:

  - The idl files in $DDS\_ROOT/dds/CorbaSeq directory are internally
     used by DDS and CAN NOT be included in your idl files.

  - Currently the CORBA::AnySeq is not supported since DDS does not
    support serialization of Any type data.

- The DDS entities (e.g. DomainParticipant, FooDataReader) have changed from
  *remote* IDL types to *local* IDL types and the data sequence and info
  sequence types have changed from normal IDL sequences to types with extra
  functionality required to support zero-copy reads.

   ***This change requires changes to DDS user code.***

   ##### impacts on user code
   
   1. The <Foo>ZCSeq and ::TAO::DCPS::SampleInfoZCSeq no longer exist.
        The <Foo>Seq and DDS::SampleInfoSeq have been changed
        to support both single-copy reads (as before) and zero-copy reads
        (like <Foo>ZCSeq should have).

        Change ::TAO::DCPS::SampleInfoZCSeq to ::DDS::SampleInfoSeq.
        Change <Foo>ZCSeq to [Modlue::]<Foo>Seq.

        The default constructor for <Foo>Seq enables zero-copy reads.
        Constructing a <Foo>Seq(num) with num > 0 enables single-copy reads.

   2. Listeners will now inherit from DDS::<enity>Listner instead of
        POA\_DDS::<entity> or POA\_TAO::DCPS::DataReader/WriterListener and
        should inherit as a local object (not a servant).  DDS provides a
        helper template that adds reference counting.  Use of this template
        (as shown below) is highly recommended.
   
      For example:

            class DataReaderListenerImpl
            : public virtual POA\_DDS::DataReaderListener,
                    public virtual PortableServer::RefCountServantBase
            {...};

      Will become:

            class DataReaderListenerImpl
            : public virtual TAO::DCPS::LocalObject<DDS::DataReaderListener>
            {...};

      But the header and implementation should require no other changes.

      **NOTE: if you are implementing the TAO specific reconnection callbacks,
      on\*disconnected, on\_\*\_reconnected, on\_\*\_lost, on\_connection\_deleted
      then you should change inheritance from
            POA\_TAO::DCPS::DataReaderListener
                to
            TAO::DCPS::LocalObject\<TAO::DCPS::DataReaderListener\>
      and remove inheritance from PortableServer::RefCountServantBase**

   3. Implementation reference counting cleanup code is no longer used

	For example:
       
            SimpleTypeSupportImpl sts_servant = new SimpleTypeSupportImpl;
            PortableServer::ServantBase_var safe_servant = sts_servant;
    
            SimpleTypeSupport_var fts =
                    TAO::DCPS::servant_to_reference (sts_servant);
    
            if (::DDS::RETCODE_OK != fts->register_type(dp.in (), MY_TYPE))

    becomes:

            SimpleTypeSupport_var fts = new SimpleTypeSupportImpl;

            if (::DDS::RETCODE_OK != fts->register_type(dp.in (), MY_TYPE))

	**Note: Although the old style is discouraged it will still work.**


   4. Since the DCPS interfaces are now local, there is very little
       performance gain to convert from \_var to a servant pointer (using
       reference\_to\_servant).

      For example:

    ```
        ::DDS::DataWriter_var dw = pub->create_datawriter(
        topic.in (),
        dw_qos,
        ::DDS::DataWriterListener::_nil());
        
        Test::SimpleDataWriter_var foo_dw = Test::SimpleDataWriter::_narrow(dw.in ());
        
        // This is unnecessary but will still work.
        // Previously fast_dw was used to increase the
        // performance of writing samples.
        Test::SimpleDataWriterImpl* fast_dw =  
        TAO::DCPS::reference_to_servant<Test::SimpleDataWriterImpl>(foo_dw.in());
        
    ```

 Also, a pointer to the servant is no longer needed for making calls on zero-copy
  read supporting overloaded methods.

   5. If the user defined DDS type is in a module then the generated types will
      also be in that same module.

            Given a "Foo" DDS type defined in the module "Test":
                      old           new
                      --------      ------------
            type      Test::Foo     Test::Foo
            sequence  FooSeq        Test::FooSeq
            reader    FooDataReader Test::FooDataReader
            writer    FooDataWriter Test::FooDataWriter

   6. The --module option to dcps\_ts.pl is no longer supported.
           The module is set as described in point #5 above.

   7. What did not change.
        You may use the following:
        <entity>\_var, <entity>\_ptr
        <entity>::\_narrrow()    // might use this for a listener
        <entity>::\_duplicate()
        TAO::DCPS::servant\_to\_reference()
        TAO::DCPS::reference\_to\_servant()
        TAO::DCPS::deactivate() // now a no-op

##### Note: 
- if you used:
    - TAO::DCPS::servant\_to\_reference()
    - TAO::DCPS::reference\_to\_servant()
    - TAO::DCPS::deactivate\_object
- for non-DDS interfaces then you may change to:
    - remote\_reference\_to\_servant
    - servant\_to\_remote\_reference
    - deactivate\_remote\_object

##### End of local interface impact to users

  - Made the sub/pub repo id generated by DCPSInfoRepo to be unique
    per DCPSInfoRepo instance instead of being unique per domain.
    This would allow multiple domains in the same process(connect
    to the same DCPSInfoRepo instance) share the same transport.

_______________________________________________________________________________
