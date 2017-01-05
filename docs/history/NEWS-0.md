Version 0.12 of TAO DDS.
-------------------------------------------------------------------------------

#### New to this version are the following changes:

- The default participant setting for BITs is now "on".

   BIT settings need to be synchronized across all participants (including
   the InfoRepo) in a domain. A mixed setting environment may not operate
   correctly.

- DCPSInfoRepo service now has a file based persistence mechanism. This
  can be configured to actively persist the InfoRepo's current state
  and in cause of a fatal failure to resurrect the same into the
  saved state.

- Fixed a bug on registration sample marshalling when data type has
  unbounded size and registers with variable size data. The problem is
  the serialization buffer does not allocate enough memory for the
  registered sample.

- It is now possible to build DDS without Built-In-Topic (BIT) support in order
  to reduce the footprint of DDS.
    
    - Generate the makefiles/projects like:
    
      mwc.pl -type <yourtype> -features built\_in\_topics=0 DDS.mwc
    
     Also, if <yourtype> happens to be gnuace, add built\_in\_topics=0 to the
  platform\_macros.GNU file or the MAKEFLAGS environment variable.

  If you are building against TAO 1.5a (patch level "0") and are NOT disabling
  the built\_in\_topics, you will need to add built\_in\_topics=1 to the
  platform\_macros.GNU file or the MAKEFLAGS environment variable since the
  inclusion of rules.dds.GNU does not work with that version of ACE+TAO.

- The reliable multicast transport was added. See the DDS developers guide
  for details.
  
  NOTE: The reliable multicast (unlike the unreliable multicast) transport
        currently doesn't provide a default configuration.  The use of the
        transport IDs TAO::DCPS::DEFAULT\_RELIABLE\_MULTICAST\_SUB\_ID and
        TAO::DCPS::DEFAULT\_RELIABLE\_MULTICAST\_PUB\_ID is unsupported.

- Fixed an issue with GCC 4 "hidden visibility" for shared libraries.

- Support for Built-In-Topics and the TRANSIENT\_LOCAL value for DURABILITY QoS
  is now complete.  This means that BITs will now work as specified.

- Implemented support for zero-copy read.

  Per the spec, when the sequences given to read/take have max\_len = 0
  the samples are not copied but are loaned to the application.

  Each data type has a <type>Seq and a <type>ZCSeq sequence.
  The <type>ZCSeq supports zero-copy reads but the <type>Seq does not.

  The default <type>ZCSeq constructor enables zero-copy read/take.
  The first parameter of the constructor defines the max\_len.
  If max\_len == 0 then zero-copy reading/taking is enabled.
  If max\_len > 0 then single-copy reading/taking is enabled.
  
  See dds/DCPS/ZeroCopySeq\_T.h for more details.

  In the next release there will be just one data sequence type
  per DDS type, <type>Seq, that supports single-copy and
  zero-copy reads (like <type>ZCSeq).  This will simplify the
  implementation of application code and make zero-copy read
  the default read/take implementation.

  Performance note: Testing has shown that samples of size 8KB or
  less do not benefit from using zero-copy reads but there is no
  negative impact on performance when using zero-copy reads for
  smaller sized samples.

  Note: zero-copy read is supported by the read and take methods but
  it should also be supported by the read\_instance and take\_instance
  methods.  This will be fixed in the next release.

_______________________________________________________________________________

Version 0.11 of TAO DDS.
-------------------------------------------------------------------------------

##### New to this version are the following changes:

- None, Version 0.11 was just a re-release of 0.10 with the version numbers
  corrected.

_______________________________________________________________________________

Version 0.10 of TAO DDS.
-------------------------------------------------------------------------------

##### New to this version are the following changes:

- The DCPSInfoRepo executable is now moved to $DDS\_ROOT/bin.  It was previously
  located in $DDS\_ROOT/dds/InfoRepo.

- Allow configurations to specify a transport that is not in a loaded
  library.  This will print a warning to ACE\_ERROR but will not fail
  until the user attempts to attach the transport to a reader/writer.
  This is useful for tests, we can now unify sub.ini and sub\_udp.ini,
  for example.

- Fix a deadlock in the DCPSInfoRepo server related to
  nested CORBA upcalls.

- Several memory leak fixes.

- A new dedicated task to cleanup resources upon remote DataLink
  disconnection. This is used among other places for cleaning
  up resources upon remote side crash.

- New configuration option 'passive\_connect\_duration' to set
  timeout on initial passive connection establishment. Setting
  this prevents deadlock when the InfoRepo directs subscriber
  to connect with a bogus/dead publisher. See the DDS Programmer's
  Guide for details.

- Fix compile issues on Green Hills compiler.

- Decrease core DDS library footprint by breaking up SimpleTCP transport
  into a separate library.

- Enhance the previous logging mechanism. Add log levels. Logging
  can be entirely compiled out for increased performance. See the
  DDS Programmer's Guide for details.

- Fix race condition involving Reconnection task.
  Fix race-condition in sending graceful disconnect message.
  Fix race condition in 'send\_delayed\_notifications'.

- Increase performance by making thread synchronization more
  fine grained.

- Supported an unreliable multicast transport -  SimpleMcast.

- Refactored SimpleUdp and SimpleMcast and now the SimpleUnreliableDgram
  library supports both SimpleUdp and SimpleMcast transports.

- Moved mpb files from the top-level DDS directory,
  into MPC/config, to make them consistent with
  the location of similar files in TAO and CIAO.
  Now the TAO version has to have an
  $ACE\_wrappers/bin/MakeProjectCreator/config/MPC.cfg to
  make MPC run success for DDS.

- Fixed memory grow in the thread-per-connection when datawriter writes too
  fast and continuously requests remove\_sample. The problem is the samples
  in thread-per-connection queue are not removed and cause memory grow.

- Supported the datareader and datawriter restart up without shutdown transport.

  Made the DCPS entity servants deleted upon delete\_*()
  call instead of upon orb shutdown. This would make the create/delete
  DCPS entity multiple times in a single process without shutdown
  Service Participant.

  The reconnect thread and connection object memory leaking problem was fixed.
  A dedicated task belonging to the TransportImpl object is used to clean up
  any resources associated with the DataLink.

  Added keep\_link configuration to indicate whether a datalink should
  be maintained when all associations are removed. This would allow
  the datareader/datawriter restart in same endpoint reconnect reuse
  the same connection.

- Fixed a problem that the send\_start could send on a nil datalink.
  The problem is a datalink insertion and finding are not sychnorized.

- Supported the default transport id for SimpleTcp and SimpleUdp.
  The range 0xFFFFFF00 to 0xFFFFFFFF is reserved for
  DEFAULT\_<transport>\_CONFIG values. Each specific transport
  has its default id in that range such as DEFAULT\_SIMPLE\_TCP\_ID = 0xFFFFFF00
  and DEFAULT\_SIMPLE\_UDP\_ID = 0xFFFFFF01. Note the default transports can not
  be configured by the users.

- Added on\_connection\_deleted to TAO::DCPS::DataWriterListener/DataReaderListener
  for testing connection object memory leak.

- Fixed a bug that uses a platform dependent type - size\_t in FULLY\_ASSOICIATION
  message. This would make publisher and subscriber not fully associated
  in inter-host test such as tests between windowsXP and Linux.

##### Known problem

1. $DDS\_ROOT/DevGuideExamples/DDS/Messenger/run\_*\_test.pl is not configured
correctly for SimpleTCP. $DDS\_ROOT/DevGuideExamples/DDS/Messenger/run\_test.pl,
however, is set up for the new SimpleTCP implementation.

2. With certain combinations of DDS version, TAO version, and compiler
version, singletons stopped working across shared-lib boundaries (code from
each shared lib would get a different singleton).

- Note: #1 and #2 are resolved in the next release.

_______________________________________________________________________________

Version 0.9 of TAO DDS.
-------------------------------------------------------------------------------

##### New to this version are the following changes:

  - Supported DDS build on VS6, gcc 4.1 and gcc 3.3.3.

  - Fixed a potential deadlock in SimpleTcpReconnectTask which could
    happen when shutdown() is invoked by the reconnect thread.

  - Fixed a deadlock situation on linux when the direct\_send()
    detects a lost connection and calls the relink(). The problem is
    a non-recursive lock in send strategy is recursively acquired.

  - Added fully association establishment support. This should fix the
    problem:
    
    
      "create\_meteorite() does not wait for full association establishment
      so even if there are waiting subscribers,  a write shortly after
      creating the datawriter may be dropped on the publisher side because
      add\_associations() has not completed on the publisher side.
      The sample may also be dropped on the subscriber side because the
      connection exists between the publisher and subscriber but the
      associations have not been mapped on the subscriber side."
	
	
  - Supported thread-per-connection send.
   
     - A new configuration "thread\_per\_connection" is added to turn on and turn
     off(default) this new feature. With this new feature, the samples are sent
     to different connections in parallel .
     
     ```
      # Determine if the transport needs create a new thread for a
      # datalink/connection to send samples under normal traffic (no backpressure).
      # If thread_per_connection is 1, the publisher creates a supered thread
      # for each datalink/connection to send samples instead of using the same
      # thread to send to different dealing's/connections sequentially.
      # The default value is 0 (using same thread send to all datalinks)
      thread_per_connection=
     ```

##### More notes and limitations

  - Current transport on receiving side does not handle a packet with
  a single control message with no data such as GRACEFUL\_DISCONNECT
  message. To work around with this problem, some bogus data is sent
  with the message.

##### Status of nightly build tests

  - The built-in topic tests failure are expected since the builtin topic was not
    supported and tested.

  - tests failed intermittently 
  
  ```  
  tests/DCPS/Compiler/idl_test1_main/run_test.pl
  [Details] ERROR: <idl_test1> exited with coredump from signal 11 : SEXY
    
  tests/DCPS/FooTest5_0/run_test.pl udp
  [Details] (24994|1084229984) DataReaderImpl::remove_associations: 
   ERROR: Unable to remove writer publisher_id 5.
  [Details] (24994|1084229984) ERROR: DataReaderImpl::add_associations, invalid 
  liveliness_changed_status inactive count.
    
  tests/DCPS/Reconnect/run_test.pl bp_timeout
[Details] test FAILED. Please see the /tao\_builds/taoadmin/BETA/ACE_wrappers/
TAO/DDS/tests/DCPS/Reconnect/test.log for details.
[Details] Error: tests/DCPS/Reconnect/run_test.pl bp_timeout returned with
 status 2
  ```
  
_______________________________________________________________________________

Version 0.8 of TAO DDS.
-------------------------------------------------------------------------------

##### New to this version are the following changes:

  - Added configuration of DCPS and transports via configuration files.
    This also simplified the code related to creating and configuring
    transports.
    See file://TAO\_DDS\_Configuration\_Settings.html for more information.

  - Fixed a possible deadlock.
  
    The scenario it was found in was:
    
    - Start a subscriber with two readers.
    - Start a publisher with two corresponding writers.
    - The subscriber would deadlock between the ORB and transport
      threads.
    - If the publisher started first, it would not deadlock.

  - Changed some heap allocations to cached allocator allocations.
    No heap allocations are in the write -> read code path for
    value types containing scalars (no arrays, sequences or strings).

  - Uninlined some unnecessary inlines to reduce the footprint.

  - Added DDS\_HAS\_MINIMUM\_BIT macro to build without Built In Topic
    support. Defining the macro will NOT build BIT support.

  - Made the DCPSInfoRepo register with IOR table as well as providing
    the ior file to provide alternate schemes of distributing the object
    reference and made the DCPSInfoRepo object reference to be persistent
    to simplify deployment.

  - Added an option -DCPSInfoRepo to replace -DCPSInfo option and deprecated
    -DCPSInfo option.

  - Fixed a bug in Registered\_Data\_Types that caused multiple domain
   participants within the same domain in a process to not register same data
   types.

  - Fixed DDS application failure between the publisher and subscriber on
   different endianess machines. The byte order is set upon data serializing and
    de-serializing. The swap\_bytes configuration value per TransportImpl
     instance is now appropriately used to allow different transports use
      different setting of swap\_bytes.

  - Added TypeSupport::get\_type\_name() support. If a nil type name is supplied
   by user to register type support then the default type name - the interface
    repository id is used.

  - Supported the SimpleTCP connection re-establishment and
   disconnected/lost/reconnected publication and subscription callbacks.
   The disconnected callback is called just before reconnect is attempted.
   If reconnect fails, the connection lost callback is called.
   If reconnect succeeds, the reconnected callback is called. 
   During reconnecting, the messages are queued until the reconnecting is done.


    If the reconnecting succeeds, the queued messages are sent out.
    Otherwise the messages are discarded.

    The main changes are:

 - Added new idl interfaces 
   - TAO::DataReaderListener and TAO::DataWriterListener to support 
   on\_\*\_disconnected, on\_\*\_lost and on\_*\_reconnected callbacks.

   - New configuration values are added to support the reconnect strategy.

	```
    ###=== Configurations for connector side during reconnecting===
    
    # The initial retry delay in milliseconds.
    # The first connection retry will be when the loss of connection
    # is detected.  The second try will be after this delay.
    # The default is 500 miliseconds.
    conn_retry_initial_delay=
    
    # The backoff multiplier for reconnection strategy.
    # The third and so on reconnect will be this value * the previous delay.
    # Hence with conn_retry_initial_delay=500 and conn_retry_backoff_multiplier=1.5
    # the second reconnect attempt will be at 0.5 seconds after first retry connect
    # fails; the third attempt will be 0.75 seconds after the second retry connect
    # fails; the fourth attempt will be 1.125 seconds after the third retry connect
    # fails.
    # The default value is 2.0.
    conn_retry_backoff_multiplier=
    
    # Number of attemps to reconnect before giving up and calling
    # on_publication_lost() and on_subscription_lost() callbacks.
    # The default is 3.
    conn_retry_attempts=
    
    # Maximum period (in milliseconds) of not being able to send queued
    # messages. If there are samples queued and no output for longer
    # than this period then the connection will be closed and on\_*\_lost()
    # callbacks will be called. If the value is zero, the default, then
    # this check will not be made.
    max_output_pause_period=
    
    ###=== Configurations for acceptor side during reconnecting===
    
    # The time period in milliseconds for the acceptor side
    # of a connection to wait for the connection to be reconnected.
    # If not reconnected within this period then
    # on\_publication\_lost() and on\_subscription\_lost() callbacks
    # will be called.
    # The default is 2 seconds (2000 millseconds).
    passive_reconnect_duration=
    ```
    
##### Implemenation details:

- A new interface - relink() is added to both receive
    and send strategy. The relink () is called whenever send or
    recv call fails. The acceptor/connector role of a connection
    object represents does not change during reconnecting. The
    connector side actively tries to reconnect and the acceptor
    side passively waits for the new connection coming.
    
  - Added a new test $DDS\_ROOT/tests/DCPS/Reconnect to test
    the on\_*\_lost callback and the connection re-establishment.

  - Fixed a problem that add\_association() failed when multiple
    publishers/subscribers start simultaneously. Added a lock
    to protect the internal map in DataLinkSetMap to fix the
    problem.

##### More notes and limitations

  - This section adds on to the "KNOWN LIMITATIONS" section at the bottom
  of this document.

  - A transport instance may only be used by publications and subscriptions
    of a single domain because the pub & sub ids are only unique within
    a domain and the transport uses this ids to map associations to
    links/connections.

  - The current transport implementation does not allow associations between
    datareaders and datawriters associated with the same transport in the same
    process. To work around this limitation, the datareader and the datawriter
    need use different TransportImpl objects. Otherwise, there are such errors:
	
	```
    (1428|2740) ERROR: RepoId (9) already exists in RepoIdSet for RepoId (8).
    (1428|2740) ERROR: Failed to insert local subscriber_id (8) to remote publisher_id (9)
    ```
    
_______________________________________________________________________________

Version 0.7 of TAO DDS.
-------------------------------------------------------------------------------

##### New to this version are the following changes:

  - Added a configuration parameter to control the use of the Nagle algorithm 
  for the SimpleTCP protocol.  By default, the Nagle algorithm is now disabled.
  - Improved VC6 support.
  - Fixed some memory leaks.
  - Added support for 64 bit architectures.

_______________________________________________________________________________

Version 0.6 of TAO DDS.
-------------------------------------------------------------------------------

##### New to this version are the following changes:

  - Changed the default ior file name for DCPSInfoRepo from dcps\_ir.ior
    to repo.ior.
  - Changed the packet and sample length fields to 32 bit integers so
    we can support samples larger than 64 KB.
  - Improved support for MS VC6

_______________________________________________________________________________

Version 0.5 of TAO DDS.
-------------------------------------------------------------------------------

##### New to this version are the following changes:

- Many memory leaks have been removed.

- A memory allocation bug has been corrected.

- The -a option of DCPSInfoRepo now works as documented.

- **This is the first release of a Data Distribution Service (DDS).**

- **See the User's Guide for an user introduction to the service.** 

- TAO's DDS implementation consists of the following:

    - Implementation of the interfaces in the DDS spec and $DDS\_ROOT/dds/DCPS
    
    - A service repository driver in $DDS\_ROOT/dds/InfoRepo
    
    - Type support generator, dcps\_ts.pl, is in $DDS\_ROOT/bin and its
     templates are in $DDS\_ROOT/bin/DCPS.
    
    - You must use DDS with a version of TAO that has had its TAO\_IDL compiler
     modified to support DDS type definition using the -Gdcps option. 
     Any OCI version later than TAO 1.4a P4 or DOC group version later than
      1.4.6 should work.
    
- An example in $DDS\_ROOT/DevGuideExamples/DDS

- Functional tests in $DDS\_ROOT/tests

- Performance tests in $DDS\_ROOT/performance-tests

_______________________________________________________________________________

