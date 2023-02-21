.. _introduction--introduction:

############
Introduction
############

..
    Sect<1>

OpenDDS is an open source implementation of the OMG Data Distribution Service (DDS) for Real-Time Systems Specification v1.4 (OMG Document ``formal/2015-04-10``) and the Real-time Publish-Subscribe Wire Protocol DDS Interoperability Wire Protocol Specification (DDSI-RTPS) v2.3 (OMG Document ``formal/2019-04-03``).
OpenDDS also implements the DDS Security Specification v1.1 (OMG Document ``formal/2018-04-01``) and DDS XTypes v1.3 (OMG Document ``formal/2020-02-04``).
OpenDDS is sponsored by Object Computing, Inc. (OCI) and is available at https://www.opendds.org/.
This Developer’s Guide is based on the version 3.23 release of OpenDDS.

DDS defines a service for efficiently distributing application data between participants in a distributed application.
This service is not tied to CORBA.
The specification provides a Platform Independent Model (PIM) as well as a Platform Specific Model (PSM) that maps the PIM onto an OMG IDL implementation.

For additional details about DDS, developers should refer to the DDS specification (OMG Document ``formal/2015-04-10``) as it contains in-depth coverage of all the service’s features.

OpenDDS is the open-source C++ implementation of OMG’s DDS specification developed and commercially supported by Object Computing, Inc. (OCI).
It is available for download from https://www.opendds.org/downloads.html and is compatible with the latest patch level of OCI TAO version 2.2a (includes ACE), and the latest DOC Group release of ACE+TAO in the ACE 6.x / TAO 2.x series.

.. note:: OpenDDS currently implements the OMG DDS version 1.4 specification.
  See the compliance information in or at https://www.opendds.org/ for more information.

.. _introduction--dcps-overview:

*************
DCPS Overview
*************

..
    Sect<1.1>

In this section we introduce the main concepts and entities of the DCPS layer and discuss how they interact and work together.

.. _introduction--basic-concepts:

Basic Concepts
==============

..
    Sect<1.1.1>

:ref:`Figure 1-1 <introduction--reffigure0>` shows an overview of the DDS DCPS layer.
The following subsections define the concepts shown in this diagram.

.. _introduction--reffigure0:

**Figure  DCPS Conceptual Overview**

.. image:: images/10000001000001C100000202637D36545E22157D.png

.. _introduction--domain:

Domain
------

..
    Sect<1.1.1.1>

The *domain* is the fundamental partitioning unit within DCPS.
Each of the other entities belongs to a domain and can only interact with other entities in that same domain.
Application code is free to interact with multiple domains but must do so via separate entities that belong to the different domains.

.. _introduction--domainparticipant:

DomainParticipant
-----------------

..
    Sect<1.1.1.2>

A *domain participant* is the entry-point for an application to interact within a particular domain.
The domain participant is a factory for many of the objects involved in writing or reading data.

.. _introduction--topic:

Topic
-----

..
    Sect<1.1.1.3>

The *topic* is the fundamental means of interaction between publishing and subscribing applications.
Each topic has a unique name within the domain and a specific data type that it publishes.
Each topic data type can specify zero or more fields that make up its *key*.
When publishing data, the publishing process always specifies the topic.
Subscribers request data via the topic.
In DCPS terminology you publish individual data *samples* for different *instances* on a topic.
Each instance is associated with a unique value for the key.
A publishing process publishes multiple data samples on the same instance by using the same key value for each sample.

.. _introduction--datawriter:

DataWriter
----------

..
    Sect<1.1.1.4>

The *data writer* is used by the publishing application code to pass values to the DDS.
Each data writer is bound to a particular topic.
The application uses the data writer’s type-specific interface to publish samples on that topic.
The data writer is responsible for marshaling the data and passing it to the publisher for transmission.

Dynamic data writers (see section :ref:`xtypes--creating-and-using-a-dynamicdatawriter`) can be used when code generated from IDL is not available or desired.
Dynamic data writers are also type-safe, but type checking happens at runtime.

.. _introduction--publisher:

Publisher
---------

..
    Sect<1.1.1.5>

The *publisher* is responsible for taking the published data and disseminating it to all relevant subscribers in the domain.
The exact mechanism employed is left to the service implementation.

.. _introduction--subscriber:

Subscriber
----------

..
    Sect<1.1.1.6>

The *subscriber* receives the data from the publisher and passes it to any relevant data readers that are connected to it.

.. _introduction--datareader:

DataReader
----------

..
    Sect<1.1.1.7>

The *data reader* takes data from the subscriber, demarshals it into the appropriate type for that topic, and delivers the sample to the application.
Each data reader is bound to a particular topic.
The application uses the data reader’s type-specific interfaces to receive the samples.

Dynamic data readers (see section :ref:`xtypes--creating-and-using-a-dynamicdatareader`) can be used when code generated from IDL is not available or desired.
Dynamic data readers are also type-safe, but type checking happens at runtime.

.. _introduction--built-in-topics:

Built-In Topics
===============

..
    Sect<1.1.2>

The DDS specification defines a number of topics that are built-in to the DDS implementation.
Subscribing to these *built-in topics* gives application developers access to the state of the domain being used including which topics are registered, which data readers and data writers are connected and disconnected, and the QoS settings of the various entities.
While subscribed, the application receives samples indicating changes in the entities within the domain.

The following table shows the built-in topics defined within the DDS specification:

.. _introduction--reftable0:

**Table  Built-in Topics**

+----------------------+---------------------------------------------------------+
| Topic Name           | Description                                             |
+======================+=========================================================+
| ``DCPSParticipant``  | Each instance represents a domain participant.          |
+----------------------+---------------------------------------------------------+
| ``DCPSTopic``        | Each instance represents a normal (not built-in) topic. |
+----------------------+---------------------------------------------------------+
| ``DCPSPublication``  | Each instance represents a data writer.                 |
+----------------------+---------------------------------------------------------+
| ``DCPSSubscription`` | Each instance represents a data reader.                 |
+----------------------+---------------------------------------------------------+

.. _introduction--quality-of-service-policies:

Quality of Service Policies
===========================

..
    Sect<1.1.3>

The DDS specification defines a number of Quality of Service (QoS) policies that are used by applications to specify their QoS requirements to the service.
Participants specify what behavior they require from the service and the service decides how to achieve these behaviors.
These policies can be applied to the various DCPS entities (topic, data writer, data reader, publisher, subscriber, domain participant) although not all policies are valid for all types of entities.

Subscribers and publishers are matched using a request-versus-offered (RxO) model.
Subscribers *request* a set of policies that are minimally required.
Publishers *offer* a set of QoS policies to potential subscribers.
The DDS implementation then attempts to match the requested policies with the offered policies; if these policies are compatible then the association is formed.

The QoS policies currently implemented by OpenDDS are discussed in detail in Chapter :ref:`quality_of_service--quality-of-service`.

.. _introduction--listeners:

Listeners
=========

..
    Sect<1.1.4>

The DCPS layer defines a callback interface for each entity that allows an application processes to “listen” for certain state changes or events pertaining to that entity.
For example, a Data Reader Listener is notified when there are data values available for reading.

.. _introduction--conditions:

Conditions
==========

..
    Sect<1.1.5>

*Conditions* and *Wait Sets* allow an alternative to listeners in detecting events of interest in DDS.
The general pattern is

The application creates a specific kind of ``Condition`` object, such as a ``StatusCondition``, and attaches it to a ``WaitSet``.

* The application waits on the ``WaitSet`` until one or more conditions become true.

* The application calls operations on the corresponding entity objects to extract the necessary information.

* The ``DataReader`` interface also has operations that take a ``ReadCondition`` argument.

* ``QueryCondition`` objects are provided as part of the implementation of the Content-Subscription Profile.
  The ``QueryCondition`` interface extends the ``ReadCondition`` interface.

.. _introduction--opendds-implementation:

**********************
OpenDDS Implementation
**********************

..
    Sect<1.2>

.. _introduction--compliance:

Compliance
==========

..
    Sect<1.2.1>

OpenDDS complies with the OMG DDS and the OMG DDSI-RTPS specifications.
Details of that compliance follows here.
OpenDDS also implements the OMG DDS Security specification.
Details of compliance to that specification are in section :ref:`dds_security--dds-security-implementation-status`.
Details of XTypes compliance are in sections :ref:`xtypes--unimplemented-features` and :ref:`xtypes--differences-from-the-specification`.

.. _introduction--dds-compliance:

DDS Compliance
--------------

..
    Sect<1.2.1.1>

Section 2 of the DDS specification defines five compliance points for a DDS implementation:

* Minimum Profile

* Content-Subscription Profile

* Persistence Profile

* Ownership Profile

* Object Model Profile

OpenDDS complies with the entire DDS specification (including all optional profiles).
This includes the implementation of all Quality of Service policies with the following notes:

* RELIABILITY.kind = RELIABLE is supported by the RTPS_UDP transport, the TCP transport, or the IP Multicast transport (when configured as reliable).

* TRANSPORT_PRIORITY is not implemented as changeable.

Although version 1.5 of the DDS specification is not yet published, OpenDDS incorporates some changes planned for that version that are required for a robust implementation:

* DDS15-257: The IDL type BuiltinTopicKey_t is a struct containing an array of 16 octets

.. _introduction--ddsi-rtps-compliance:

DDSI-RTPS Compliance
--------------------

..
    Sect<1.2.1.2>

The OpenDDS implementation complies with the requirements of the OMG DDSI-RTPS specification.

.. _introduction--opendds-rtps-implementation-notes:

OpenDDS RTPS Implementation Notes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

..
    Sect<1.2.1.2.1>

The OMG DDSI-RTPS specification (formal/2019-04-03) supplies statements for implementation, but not required for compliance.
The following items should be taken into consideration when utilizing the OpenDDS RTPS functionality for transport and/or discovery.
Section numbers of the DDSI-RTPS specification are supplied with each item for further reference.

Items not implemented in OpenDDS:

#. Writer-side content filtering (8.7.3)OpenDDS may still drop samples that aren't needed (due to content filtering) by any associated readers — this is done above the transport layer

#. Coherent sets for ``PRESENTATION`` QoS (8.7.5)

#. Directed writes (8.7.6)

   * OpenDDS will use the Directed Write parameter if it’s present on incoming messages (for example, messages generated by a different DDS implementation)

#. Property lists (8.7.7)

#. Original writer info for ``DURABLE`` data (8.7.8) -- this would only be used for transient and persistent durability, which are not supported by the RTPS specification (8.7.2.2.1)

#. Key Hashes (8.7.9) are not generated, but they are optional

#. ``nackSuppressionDuration`` (Table 8.47) and ``heartbeatSuppressionDuration`` (Table 8.62).

.. note:: Items 3 and 4 above are described in the DDSI-RTPS specification.
  However, they do not have a corresponding concept in the DDS specification.

.. _introduction--idl-compliance:

IDL Compliance
--------------

..
    Sect<1.2.1.3>

OMG IDL is used in a few different ways in the OpenDDS code base and downstream applications that use it:

* Files that come with OpenDDS such as :ghfile:`dds/DdsDcpsTopic.idl` define parts of the API between the middleware libraries and the application.
  This is known as the OMG IDL Platform Specific Model (PSM).

* Users of OpenDDS author IDL files in addition to source code files in C++ or Java.

This section only describes the latter use.

The IDL specification (version 4.2) uses the term “building block” to define subsets of the overall IDL grammar that may be supported by certain tools.
OpenDDS supports the following building blocks, with notes/caveats listed below each:

* Core Data Types

  * Support for the “fixed” data type (fixed point decimal) is incomplete.

* Anonymous Types

  * There is limited support for anonymous types when they appear as sequence/array instantiations directly as struct field types.
    Using an explicitly-named type is recommended.

* Annotations

  * See sections :ref:`getting_started--defining-data-types-with-idl` and :ref:`xtypes--idl-annotations` for details on which built-in annotations are supported.

  * User-defined annotation types are also supported.

* Extended Data Types

  * The integer types ``int8``, ``uint8``, ``int16``, ``uin16``, ``int32`` ``uint32``, ``int64``, and ``uint64`` are supported.

  * The rest of the building block is not supported.

.. _introduction--extensions-to-the-dds-specification:

Extensions to the DDS Specification
===================================

..
    Sect<1.2.2>

Data types, interfaces, and constants in the **DDS** IDL module (C++ namespace, Java package) correspond directly to the DDS specification with very few exceptions:

* ``DDS::SampleInfo`` contains an extra field starting with ``opendds_reserved``.

* Type-specific DataReaders (including those for Built-in Topics) have additional operations ``read_instance_w_condition()`` and ``take_instance_w_condition()``.

Additional extended behavior is provided by various classes and interfaces in the OpenDDS module/namespace/package.
Those include features like Recorder and Replayer (see chapter :ref:`alternate_interfaces_to_data--alternate-interfaces-to-data`) and also:

* ``OpenDDS::DCPS::TypeSupport`` adds the ``unregister_type()`` operation not found in the DDS spec.

* ``OpenDDS::DCPS::ALL_STATUS_MASK``, ``NO_STATUS_MASK``, and ``DEFAULT_STATUS_MASK`` are useful constants for the ``DDS::StatusMask`` type used by ``DDS::Entity``, ``DDS::StatusCondition``, and the various ``create_*()`` operations.

.. _introduction--opendds-architecture:

OpenDDS Architecture
====================

..
    Sect<1.2.3>

This section gives a brief overview of the OpenDDS implementation, its features, and some of its components.
The ``$DDS_ROOT`` environment variable should point to the base directory of the OpenDDS distribution.
Source code for OpenDDS can be found under the :ghfile:`dds/` directory.
Tests can be found under :ghfile:`tests/`.

.. _introduction--design-philosophy:

Design Philosophy
-----------------

..
    Sect<1.2.3.1>

The OpenDDS implementation and API is based on a fairly strict interpretation of the OMG IDL PSM.
In almost all cases the OMG’s IDL-to-C++ Language Mapping is used to define how the IDL in the DDS specification is mapped into the C++ APIs that OpenDDS exposes to the client.

The main deviation from the OMG IDL PSM is that local interfaces are used for the entities and various other interfaces.
These are defined as unconstrained (non-local) interfaces in the DDS specification.
Defining them as local interfaces improves performance, reduces memory usage, simplifies the client’s interaction with these interfaces, and makes it easier for clients to build their own implementations.

.. _introduction--extensible-transport-framework-etf:

Extensible Transport Framework (ETF)
------------------------------------

..
    Sect<1.2.3.2>

OpenDDS uses the IDL interfaces defined by the DDS specification to initialize and control service usage.
Data transmission is accomplished via an OpenDDS-specific transport framework that allows the service to be used with a variety of transport protocols.
This is referred to as *pluggable transports* and makes the extensibility of OpenDDS an important part of its architecture.
OpenDDS currently supports TCP/IP, UDP/IP, IP multicast, shared-memory, and RTPS_UDP transport protocols as shown in :ref:`Figure 1-2 <introduction--reffigure1>`.
Transports are typically specified via configuration files and are attached to various entities in the publisher and subscriber processes.
Refer to Section :ref:`run_time_configuration--transport-configuration-options` for details on configuring ETF components.

.. _introduction--reffigure1:

.. image:: images/10000001000002E50000018D97FADEED4445DDBB.png

**Figure  OpenDDS Extensible Transport Framework**

The ETF enables application developers to implement their own customized transports.
Implementing a custom transport involves specializing a number of classes defined in the transport framework.
The ``udp`` transport provides a good foundation developers may use when creating their own implementation.
See the :ghfile:`dds/DCPS/transport/udp/` directory for details.

.. _introduction--dds-discovery:

DDS Discovery
-------------

..
    Sect<1.2.3.3>

DDS applications must discover one another via some central agent or through some distributed scheme.
An important feature of OpenDDS is that DDS applications can be configured to perform discovery using the DCPSInfoRepo or RTPS discovery, but utilize a different transport type for data transfer between data writers and data readers.
The OMG DDS specification (``formal/2015-04-10``) leaves the details of discovery to the implementation.
In the case of interoperability between DDS implementations, the OMG DDSI-RTPS ``(formal/2014-09-01)`` specification provides requirements for a peer-to-peer style of discovery.

OpenDDS provides two options for discovery.

#. Information Repository: a centralized repository style that runs as a separate process allowing publishers and subscribers to discover one another centrally or

#. RTPS Discovery: a peer-to-peer style of discovery that utilizes the RTPS protocol to advertise availability and location information.

Interoperability with other DDS implementations must utilize the peer-to-peer method, but can be useful in OpenDDS-only deployments.

.. _introduction--centralized-discovery-with-dcpsinforepo:

Centralized Discovery with DCPSInfoRepo
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

..
    Sect<1.2.3.3.1>

OpenDDS implements a standalone service called the DCPS Information Repository (DCPSInfoRepo) to achieve the centralized discovery method.
It is implemented as a CORBA server.
When a client requests a subscription for a topic, the DCPS Information Repository locates the topic and notifies any existing publishers of the location of the new subscriber.
The DCPSInfoRepo process needs to be running whenever OpenDDS is being used in a non-RTPS configuration.
An RTPS configuration does not use the DCPSInfoRepo.
The DCPSInfoRepo is not involved in data propagation, its role is limited in scope to OpenDDS applications discovering one another.

.. _introduction--reffigure2:

.. image:: images/100000010000045A0000025185A3A43482F62E3D.png

**Figure : Centralized Discovery with OpenDDS InfoRepo**

Application developers are free to run multiple information repositories with each managing their own non-overlapping sets of DCPS domains.

It is also possible to operate domains with more than a single repository, thus forming a distributed virtual repository.
This is known as *Repository Federation*.
In order for individual repositories to participate in a federation, each one must specify its own federation identifier value (a 32-bit numeric value) upon start-up.
See :ref:`the_dcps_information_repository--repository-federation` for further information about repository federations.

.. _introduction--peer-to-peer-discovery-with-rtps:

Peer-to-Peer Discovery with RTPS
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

..
    Sect<1.2.3.3.2>

DDS applications requiring a Peer-to-Peer discovery pattern can be accommodated by OpenDDS capabilities.
This style of discovery is accomplished only through the use of the RTPS protocol as of the current release.
This simple form of discovery is accomplished through simple configuration of DDS application data readers and data writers running in application processes as shown in :ref:`Figure 1-4 <introduction--reffigure3>`.
As each participating process activates the DDSI-RTPS discovery mechanisms in OpenDDS for their data readers and writers, network endpoints are created with either default or configured network ports such that DDS participants can begin advertising the availability of their data readers and data writers.
After a period of time, those seeking one another based on criteria will find each other and establish a connection based on the configured pluggable transport as discussed in Extensible Transport Framework (ETF).
A more detailed description of this flexible configuration approach is discussed in Section :ref:`run_time_configuration--transport-concepts` and Section :ref:`run_time_configuration--rtps-udp-transport-configuration-options`.

.. _introduction--reffigure3:

.. image:: images/10000001000003FC0000025E8CF71A4C4FCDEFF3.png

**Figure : Peer-to-peer Discovery with RTPS**

The following are additional implementation limits that developers need to take into consideration when developing and deploying applications that use RTPS discovery:

#. Domain IDs should be between 0 and 231 (inclusive) due to the way UDP ports are assigned to domain IDs.
   In each OpenDDS process, up to 120 domain participants are supported in each domain.

#. Topic names and type identifiers are limited to 256 characters.

#. OpenDDS's native multicast transport does not work with RTPS Discovery due to the way GUIDs are assigned (a warning will be issued if this is attempted).

For more details in how RTPS discovery occurs, a very good reference to read can be found in Section 8.5 of the Real-time Publish-Subscribe Wire Protocol DDS Interoperability Wire Protocol Specification (DDSI-RTPS) v2.2 (OMG Document ``formal/2014-09-01``).

.. _introduction--threading:

Threading
---------

..
    Sect<1.2.3.4>

OpenDDS creates its own ORB (when one is required) as well as a separate thread upon which to run that ORB.
It also uses its own threads to process incoming and outgoing transport I/O.
A separate thread is created to cleanup resources upon unexpected connection closure.
Your application may get called back from these threads via the Listener mechanism of DCPS.

When publishing a sample via DDS, OpenDDS normally attempts to send the sample to any connected subscribers using the calling thread.
If the send call blocks, then the sample may be queued for sending on a separate service thread.
This behavior depends on the QoS policies described in Chapter :ref:`quality_of_service--quality-of-service`.

All incoming data in the subscriber is read by a service thread and queued for reading by the application.
DataReader listeners are called from the service thread.

.. _introduction--configuration:

Configuration
-------------

..
    Sect<1.2.3.5>

OpenDDS includes a file-based configuration framework for configuring both global items such as debug level, memory allocation, and discovery, as well as transport implementation details for publishers and subscribers.
Configuration can also be achieved directly in code, however, it is recommended that configuration be externalized for ease of maintenance and reduction in runtime errors.
The complete set of configuration options are described in Chapter :ref:`run_time_configuration--run-time-configuration`.

.. _introduction--installation:

************
Installation
************

..
    Sect<1.3>

The steps on how to build OpenDDS can be found in :ghfile:`INSTALL.md`.

To build OpenDDS with DDS Security, see section :ref:`dds_security--building-opendds-with-security-enabled` below.

To avoid compiling OpenDDS code that you will not be using, there are certain features than can be excluded from being built.
The features are discussed below.

Users requiring a small-footprint configuration or compatibility with safety-oriented platforms should consider using the OpenDDS Safety Profile, which is described in chapter :ref:`safety_profile--safety-profile` of this guide.

.. _introduction--building-with-a-feature-enabled-or-disabled:

Building With a Feature Enabled or Disabled
===========================================

..
    Sect<1.3.1>

Most features are supported by the ``configure`` script.
The ``configure`` script creates config files with the correct content and then runs MPC.
If you are using the ``configure`` script, run it with the ``--help`` command line option and look for the feature you wish to enable/disable.
If you are not using the ``configure`` script, continue reading below for instructions on running MPC directly.

For the features described below, MPC is used for enabling (the default) a feature or disabling the feature.
For a feature named *feature*, the following steps are used to disable the feature from the build:

#. Use the command line “features” argument to MPC:``mwc.pl -type <type> -featuresfeature=0 DDS.mwc`` Or alternatively, add the line ``feature=0`` to the file ``$ACE_ROOT/bin/MakeProjectCreator/config/default.features`` and regenerate the project files using MPC.

#. If you are using the ``gnuace`` MPC project type (which is the case if you will be using GNU make as your build system), add line “``feature=0``” to the file ``$ACE_ROOT/include/makeinclude/platform_macros.GNU``.

To explicitly enable the feature, use ``feature=1`` above.

.. note:: You can also use the :ghfile:`configure` script to enable or disable features.
  To disable the feature, pass ``--no-feature`` to the script, to enable pass ``--feature``.
  In this case ``-`` is used instead of ``_`` in the feature name.
  For example, to disable feature ``content_subscription`` discussed below, pass ``--no-content-subscription`` to the configure script.

.. _introduction--disabling-the-building-of-built-in-topic-support:

Disabling the Building of Built-In Topic Support
================================================

..
    Sect<1.3.2>

Feature Name: ``built_in_topics``

You can reduce the footprint of the core DDS library by up to 30% by disabling Built-in Topic Support.
See Chapter :ref:`built_in_topics--built-in-topics` for a description of Built-In Topics.

.. _introduction--disabling-the-building-of-compliance-profile-features:

Disabling the Building of Compliance Profile Features
=====================================================

..
    Sect<1.3.3>

The DDS specification defines *compliance profiles* to provide a common terminology for indicating certain feature sets that a DDS implementation may or may not support.
These profiles are given below, along with the name of the MPC feature to use to disable support for that profile or components of that profile.

Many of the profile options involve QoS settings.
If you attempt to use a QoS value that is incompatible with a disabled profile, a runtime error will occur.
If a profile involves a class, a compile time error will occur if you try to use the class and the profile is disabled.

.. _introduction--content-subscription-profile:

Content-Subscription Profile
----------------------------

..
    Sect<1.3.3.1>

Feature Name: ``content_subscription``

This profile adds the classes ``ContentFilteredTopic``, ``QueryCondition``, and ``MultiTopic`` discussed in Chapter :ref:`content_subscription_profile--content-subscription-profile`.

In addition, individual classes can be excluded by using the features given in the table below.

.. _introduction--reftable1:

**Table : Content-Subscription Class Features**

+----------------------+----------------------------+
| Class                | Feature                    |
+======================+============================+
| ContentFilteredTopic | ``content_filtered_topic`` |
+----------------------+----------------------------+
| QueryCondition       | ``query_condition``        |
+----------------------+----------------------------+
| MultiTopic           | ``multi_topic``            |
+----------------------+----------------------------+

.. _introduction--persistence-profile:

Persistence Profile
-------------------

..
    Sect<1.3.3.2>

Feature Name: ``persistence_profile``

This profile adds the QoS policy ``DURABILITY_SERVICE`` and the settings ‘``TRANSIENT``’ and ‘``PERSISTENT``’ of the ``DURABILITY`` QoS policy ``kind``.

.. _introduction--ownership-profile:

Ownership Profile
-----------------

..
    Sect<1.3.3.3>

Feature Name: ``ownership_profile``

This profile adds:

* the setting ‘``EXCLUSIVE``’ of the ``OWNERSHIP`` ``kind``

* support for the ``OWNERSHIP_STRENGTH`` policy

* setting a ``depth > 1`` for the ``HISTORY`` QoS policy.

*Some users may wish to exclude support for the Exclusive OWNERSHIP policy and its associated OWNERSHIP_STRENGTH without impacting use of HISTORY.*
*In order to support this configuration, OpenDDS also has the MPC feature ownership_kind_exclusive (configure script option --no-ownership-kind-exclusive).*

.. _introduction--object-model-profile:

Object Model Profile
--------------------

..
    Sect<1.3.3.4>

Feature Name: ``object_model_profile``

This profile includes support for the ``PRESENTATION`` access_scope setting of ‘``GROUP``’.

.. note:: Currently, the ``PRESENTATION`` access_scope of ‘``TOPIC``’ is also excluded when ``object_model_profile`` is disabled.

.. _introduction--building-applications-that-use-opendds:

**************************************
Building Applications that use OpenDDS
**************************************

..
    Sect<1.4>

This section applies to any C++ code that directly or indirectly includes OpenDDS headers.
For Java applications, see Chapter :ref:`java_bindings--java-bindings` below.

C++ source code that includes OpenDDS headers can be built using either build system: MPC or CMake.

.. _introduction--mpc-the-makefile-project-and-workspace-creator:

MPC: The Makefile, Project, and Workspace Creator
=================================================

..
    Sect<1.4.1>

OpenDDS is itself built with MPC, so development systems that are set up to use OpenDDS already have MPC available.
The OpenDDS configure script creates a “setenv” script with environment settings (``setenv.cmd`` on Windows; ``setenv.sh`` on Linux/macOS).
This environment contains the ``PATH`` and ``MPC_ROOT`` settings necessary to use MPC.

MPC’s source tree (in ``MPC_ROOT``) contains a “docs” directory with both HTML and plain text documentation (``USAGE`` and ``README`` files).

The example walk-through in section :ref:`getting_started--using-dcps` uses MPC as its build system.
The OpenDDS source tree contains many tests and examples that are built with MPC.
These can be used as starting points for application MPC files.

.. _introduction--cmake:

CMake
=====

..
    Sect<1.4.2>

Applications can also be built with `CMake <https://cmake.org/>`__.
See the included documentation in the OpenDDS source tree: :ghfile:`docs/cmake.md`

The OpenDDS source tree also includes examples of using CMake.
They are listed in the ``cmake.md`` file.

.. _introduction--custom-build-systems:

Custom Build systems
====================

..
    Sect<1.4.3>

Users of OpenDDS are strongly encouraged to select one of the two options listed above (MPC or CMake) to generate consistent build files on any supported platform.
If this is not possible, users of OpenDDS must make sure that all code generator, compiler, and linker settings in the custom build setup result in API- and ABI-compatible code.
To do this, start with an MPC or CMake-generated project file (makefile or Visual Studio project file) and make sure all relevant settings are represented in the custom build system.
This is often done through a combination of inspecting the project file and running the build with verbose output to see how the toolchain (code generators, compiler, linker) is invoked.
Contact Object Computing, Inc. (OCI) via https://objectcomputing.com/products/opendds/consulting-support to have our expert software engineers work on this for you.

