.. _introduction:

#######################
Introduction to OpenDDS
#######################

.. _introduction--what-is-opendds:

****************
What is OpenDDS?
****************

..
    Sect<0.1>
    Sect<0.6>

OpenDDS is an open-source C++ framework for exchanging data in distributed systems.
It is an implementation of :ref:`a group of related OMG specifications <specs>`.
OpenDDS is implemented in C++ and contains support for :doc:`Java <java_bindings>`.
Users in the OpenDDS community have contributed and maintain bindings for other languages that include `C# <https://www.openddsharp.com/>`__, `Node.js <https://github.com/OpenDDS/node-opendds>`__, and `Python <https://github.com/OpenDDS/pyopendds>`__.
OpenDDS is sponsored by the OpenDDS Foundation and is available via https://opendds.org and https://github.com/OpenDDS/OpenDDS.

***************
Licensing Terms
***************

..
    Sect<0.2>

OpenDDS is *open source software*.
The source code may be freely downloaded and is open for inspection, review, comment, and improvement.
Copies may be freely installed across all your systems and those of your customers.
There is no charge for development or run-time licenses.
The source code is designed to be compiled, and used, across a wide variety of hardware and operating systems architectures.
You may modify it for your own needs, within the terms of the license agreements.
You must not copyright OpenDDS software.
For details of the licensing terms, see the file named :ghfile:`LICENSE` that is included in the OpenDDS source code distribution or visit https://opendds.org/about/license.html.

OpenDDS also utilizes other open source software products including MPC (Make Project Creator), ACE (the ADAPTIVE Communication Environment), and TAO (The ACE ORB).

OpenDDS is open source and the development team welcomes contributions of code, tests, documentation, and ideas.
Active participation by users ensures a robust implementation.
Contact the OpenDDS Foundation if you are interested in contributing to the development of OpenDDS.
Please note that any code or documentation that is contributed to and becomes part of the OpenDDS open source code base is subject to the same licensing terms as the rest of the OpenDDS code base.

.. _specs:

**************
Specifications
**************

OpenDDS is an open source implementation of a group of related :term:`OMG` specifications.

.. _spec-dds:

Data Distribution Service (DDS) for Real-Time Systems
=====================================================

This specification defines a service for efficiently distributing application data between participants in a distributed application.
This is the core functionality implemented by OpenDDS for real-time publish and subscribe applications and is described throughout this document.

The version OpenDDS uses is :omgspec:`dds`.
Compliance with the specification is documented in :ref:`introduction--dds-compliance`.
More information about the DDS itself can be found on the `DDS Foundation website <https://www.dds-foundation.org/>`__.

.. _spec-rtps:

Real-time Publish-Subscribe (RTPS)
==================================

The full name of this specification is the *Real-time Publish-Subscribe Protocol DDS Interoperability Wire Protocol* (DDSI-RTPS), but can also be just called RTPS.
This specification describes the requirements for interoperability between DDS implementations.
See :ref:`introduction--peer-to-peer-discovery-with-rtps` for more information.

The version OpenDDS uses is :omgspec:`rtps`.
Although the document number is v2.3, it specifies protocol version 2.4.
Compliance with the specification is documented in :ref:`introduction--ddsi-rtps-compliance`.

.. _spec-dds-security:

DDS Security
============

This specification extends DDS with capabilities for authentication and encryption.
OpenDDS's support for the DDS Security specification is described in :doc:`dds_security`.

The version OpenDDS uses is :omgspec:`sec`.
Compliance with the specification is documented in :ref:`dds_security--dds-security-implementation-status`.

.. _spec-xtypes:

Extensible and Dynamic Topic Types for DDS (XTypes)
===================================================

This specification defines details of the type system used for the data exchanged on DDS Topics, including how schema and data are encoded for network transmission.
OpenDDS's support for XTypes is described in :doc:`xtypes`.

The version OpenDDS uses is :omgspec:`xtypes`.
Compliance with the specification is documented in :ref:`xtypes--unimplemented-features` and :ref:`xtypes--differences-from-the-specification`.

.. _spec-idl:

IDL
===

:term:`IDL` is a language that can be used to define data structures and interfaces that can be mapped to multiple programming languages.
The parser is implemented as part of :term:`tao_idl`.

The version OpenDDS uses is :omgspec:`idl`.
Compliance with the specification is documented in :ref:`introduction--idl-compliance`.

.. _spec-idl-to-cpp03:

IDL to C++03 Language Mapping
=============================

This specification defines an :term:`IDL` to C++ mapping.
It's generated by :term:`tao_idl`, not :term:`opendds_idl`.

The version OpenDDS uses is :omgspec:`cpp03`.

.. _spec-idl-to-cpp11:

IDL to C++11 Language Mapping
=============================

This specification defines an :term:`IDL` to C++ mapping that takes advantage of C++11 language features and standard library types.
OpenDDS's support for IDL to C++11 is described in :ref:`opendds_idl--using-the-idl-to-c-11-mapping`.

The version OpenDDS uses is :omgspec:`cpp11`.

.. _spec-idl-to-java:

IDL to Java Language Mapping
=============================

This specification defines an :term:`IDL` to Java mapping and is used for the :ref:`java`.

The version OpenDDS uses is :omgspec:`java`.

..
    Sect<1.2>

.. _introduction--compliance:

**********
Compliance
**********

..
    Sect<1.2.1>

OpenDDS complies with the OMG DDS and the OMG DDSI-RTPS specifications.
Details of that compliance follows here.
OpenDDS also implements the OMG DDS Security specification.
See :ref:`specs` for how OpenDDS complies with other specifications it implements.

.. _introduction--dds-compliance:

DDS Compliance
==============

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

* :ref:`qos-reliability` ``RELIABLE_RELIABILITY_QOS`` is supported by the RTPS_UDP transport, the TCP transport, and the IP Multicast transport (when configured as reliable).

* :ref:`qos-transport-priority` is not implemented as changeable.

Although version 1.5 of the DDS specification is not yet published, OpenDDS incorporates some changes planned for that version that are required for a robust implementation:

* :omgissue:`DDS15-5`: The IDL type ``BuiltinTopicKey_t`` is a struct containing an array of 16 octets

  * The actual child issue isn't public viewable for some reason, but the member link is https://issues.omg.org/browse/DDS15-257

.. _introduction--ddsi-rtps-compliance:

DDSI-RTPS Compliance
====================

..
    Sect<1.2.1.2>

The OpenDDS implementation complies with the requirements of the OMG DDSI-RTPS specification.

.. _introduction--opendds-rtps-implementation-notes:

OpenDDS RTPS Implementation Notes
---------------------------------

..
    Sect<1.2.1.2.1>

The :ref:`OMG DDSI-RTPS specification <spec-rtps>` supplies statements for implementation, but not required for compliance.
The following items should be taken into consideration when utilizing the OpenDDS RTPS functionality for transport and/or discovery.
Section numbers of the DDSI-RTPS specification are supplied with each item for further reference.

Items not implemented in OpenDDS:

#. Writer-side content filtering (:omgspec:`rtps:8.7.3 Content-filtered Topics`)

   OpenDDS may still drop samples that aren't needed (due to content filtering) by any associated readers -- this is done above the transport layer

#. :omgspec:`rtps:8.7.6 Coherent Sets` for :ref:`qos-presentation`

#. :omgspec:`rtps:8.7.7 Directed Write`

   OpenDDS will use the Directed Write parameter if it's present on incoming messages (for example, messages generated by a different DDS implementation)

#. :omgspec:`rtps:8.7.8 Property Lists`

#. :omgspec:`rtps:8.7.9 Original Writer Info` for :ref:`qos-durability`

   This would only be used for transient and persistent durability, which are :omgspec:`not supported by the RTPS specification <rtps:8.7.2.2.1>`

#. :omgspec:`Key Hashes <rtps:8.7.10 Key Hash>` are not generated, but the specification makes them optional

#. ``nackSuppressionDuration`` (Table 8.47 in :omgspec:`rtps:8.4.7.1 RTPS Writer`) and ``heartbeatSuppressionDuration`` (Table 8.62 in :omgspec:`rtps:8.4.10.1 RTPS Reader`).

.. note:: Items 3 and 4 above are described in the DDSI-RTPS specification.
  However, they do not have a corresponding concept in the DDS specification.

.. _introduction--idl-compliance:

IDL Compliance
==============

..
    Sect<1.2.1.3>

OMG IDL is used in a few different ways in the OpenDDS code base and downstream applications that use it:

* Files that come with OpenDDS such as :ghfile:`dds/DdsDcpsTopic.idl` define parts of the API between the middleware libraries and the application.
  This is known as the OMG IDL Platform Specific Model (PSM).

* Users of OpenDDS author IDL files in addition to source code files in C++ or Java.

This section only describes the latter use.

The IDL specification (version 4.2) uses the term "building block" to define subsets of the overall IDL grammar that may be supported by certain tools.
OpenDDS supports the following building blocks, with notes/caveats listed below each:

* Core Data Types

  * Support for the "fixed" data type (fixed point decimal) is incomplete.

* Anonymous Types

  * There is limited support for anonymous types when they appear as sequence/array instantiations directly as struct field types.
    Using an explicitly-named type is recommended.

* Annotations

  * See :ref:`getting_started--defining-data-types-with-idl` and :ref:`xtypes--idl-annotations` for details on which built-in annotations are supported.

  * User-defined annotation types are also supported.

* Extended Data Types

  * The integer types ``int8``, ``uint8``, ``int16``, ``uin16``, ``int32`` ``uint32``, ``int64``, and ``uint64`` are supported.

  * The rest of the building block is not supported.

.. _introduction--extensions-to-the-dds-specification:

***********************************
Extensions to the DDS Specification
***********************************

..
    Sect<1.2.2>

Data types, interfaces, and constants in the ``DDS`` IDL module (C++ namespace, Java package) correspond directly to the DDS specification with very few exceptions:

* ``DDS::SampleInfo`` contains an extra field starting with ``opendds_reserved``.

* Type-specific DataReaders (including those for Built-in Topics) have additional operations ``read_instance_w_condition()`` and ``take_instance_w_condition()``.

Additional extended behavior is provided by various classes and interfaces in the ``OpenDDS`` module/namespace/package.
Those include features like Recorder and Replayer (:ref:`altdata`) and also:

* ``OpenDDS::DCPS::TypeSupport`` adds the ``unregister_type()`` operation not found in the DDS spec.

* ``OpenDDS::DCPS::ALL_STATUS_MASK``, ``NO_STATUS_MASK``, and ``DEFAULT_STATUS_MASK`` are useful constants for the ``DDS::StatusMask`` type used by ``DDS::Entity``, ``DDS::StatusCondition``, and the various ``create_*()`` operations.

.. _introduction--opendds-architecture:

***************************************
OpenDDS Implementation and Architecture
***************************************

..
    Sect<1.2.3>

This section gives a brief overview of the OpenDDS implementation, its features, and some of its components.

Source Code Organization
========================

Relative to :envvar:`DDS_ROOT`:

* the :ghfile:`dds/` directory contains the source code for OpenDDS.
* the :ghfile:`tests/` directory contains tests.
* the :ghfile:`tools/` directory contains tools and libraries like the DCPSInfoRepo, RtpsRelay, and the Modeling SDK.
* the :ghfile:`DevGuideExamples/` directory contains examples used in this guide.
* the :ghfile:`examples/` directory contains examples *not* used in this guide.
* the :ghfile:`docs/` directory contains documentation for users and developers of OpenDDS.

.. _introduction--design-philosophy:

Design Philosophy
=================

..
    Sect<1.2.3.1>

The OpenDDS implementation and API is based on a fairly strict interpretation of the OMG IDL PSM.
In almost all cases the OMG's IDL-to-C++ Language Mapping is used to define how the IDL in the DDS specification is mapped into the C++ APIs that OpenDDS exposes to the client.

The main deviation from the OMG IDL PSM is that local interfaces are used for the entities and various other interfaces.
These are defined as unconstrained (non-local) interfaces in the DDS specification.
Defining them as local interfaces improves performance, reduces memory usage, simplifies the client's interaction with these interfaces, and makes it easier for clients to build their own implementations.

.. _introduction--extensible-transport-framework-etf:

Extensible Transport Framework (ETF)
====================================

..
    Sect<1.2.3.2>

OpenDDS uses the IDL interfaces defined by the DDS specification to initialize and control service usage.
Data transmission is accomplished via an OpenDDS-specific transport framework that allows the service to be used with a variety of transport protocols.
This is referred to as *pluggable transports* and makes the extensibility of OpenDDS an important part of its architecture.
OpenDDS currently supports TCP/IP, UDP/IP, IP multicast, shared-memory, and RTPS_UDP transport protocols as shown below.

.. figure:: images/pluggable.png

  OpenDDS Transport Framework

Transports are typically specified via configuration files and are attached to various entities in the publisher and subscriber processes.
See :ref:`run_time_configuration--transport-configuration-options` for details on configuring ETF components.

The ETF enables application developers to implement their own customized transports.
Implementing a custom transport involves specializing a number of classes defined in the transport framework.
The ``udp`` transport provides a good foundation developers may use when creating their own implementation.
See the :ghfile:`dds/DCPS/transport/udp/` directory for details.

.. _introduction--dds-discovery:

Discovery
=========

..
    Sect<1.2.3.3>

DDS applications must discover one another via some central agent or through some distributed scheme (see :ref:`dds-introduction--discovery`).
OpenDDS provides three options for discovery:  :ref:`introduction--centralized-discovery-with-dcpsinforepo`, :ref:`introduction--peer-to-peer-discovery-with-rtps`, and :ref:`introduction--static-discovery`.
The choice of discovery is independent of the choice of transport in most cases.
For example, one can use the tcp transport with RTPS Discovery.
Two notable exceptions are:

#. If using DDS Security, RTPS must be used for both the transport and discovery.
#. RTPS must be used for the transport when using Static Discovery.

Like transports, additional discovery implementations can be created and plugged in.

.. _introduction--centralized-discovery-with-dcpsinforepo:

DCPSInfoRepo
------------

..
    Sect<1.2.3.3.1>

.. figure:: images/inforepo_discovery.png

   Centralized Discovery with DCPSInfoRepo

OpenDDS contains a standalone CORBA service called :ref:`inforepo`.
An instance of the DCPSInfoRepo is shared by all the participants in a domain and constitutes a centralized approach to discovery.
Each OpenDDS application connects to the DCPSInfoRepo and creates records for its participants, topics, data writers, and data readers.
As records for data writers and data readers are created, they are matched against the existing set of records.
When matches are found, the DCPSInfoRepo invokes the participant to perform the necessary associations.
The DCPSInfoRepo is not involved in data propagation; its role is limited in scope to OpenDDS applications discovering one another.
The DCPSInfoRepo populates the :ref:`introduction--built-in-topics` for a participant if configured to do so.
OpenDDS creates its own ORB and a separate thread to run that ORB when using DCPSInfoRepo discovery.
See :ref:`run_time_configuration--configuring-applications-for-dcpsinforepo` for details on how applications can be configured to use the DCPSInfoRepo.

Application developers are free to run multiple information repositories with each managing their own non-overlapping sets of DCPS domains.

It is also possible to operate domains with more than a single repository, thus forming a distributed virtual repository.
This is known as *Repository Federation*.
In order for individual repositories to participate in a federation, each one must specify its own federation identifier value (a 32-bit numeric value) upon start-up.
See :ref:`the_dcps_information_repository--repository-federation` for further information about repository federations.

.. _introduction--peer-to-peer-discovery-with-rtps:

RTPS Discovery
--------------

..
    Sect<1.2.3.3.2>

.. figure:: images/rtps_discovery.png

  Peer-to-peer Discovery with RTPS

RTPS Discovery is an implementation of the OMG DDSI-RTPS ``(formal/2014-09-01)`` specification (see :omgspec:`rtps:8.5 Discovery Module`).
RTPS Discovery uses the RTPS protocol to advertise and discover participants, data writers, and data readers.
RTPS Discovery uses multicast to discover participants and *builtin endpoints* (not to be confused with Builtin Topics).
This part of RTPS discovery is called the Simple Participant Discovery Protocol (SPDP).
After the builtin endpoints are discovered and associated, they exchange information about data writers and data readers which are called *endpoints*.
This part of RTPS discovery is called Simple Endpoint Discovery Protocol (SEDP).
RTPS Discovery is a peer-to-peer approach to discovery as each participant interacts directly with other participants to accomplish discovery.
RTPS is interoperable and supports :ref:`dds_security`.
RTPS discovery populates the Builtin Topics for a participant.
See :ref:`run_time_configuration--configuring-for-ddsi-rtps-discovery` for details on how applications can be configured to use RTPS Discovery.
See also :ref:`run_time_configuration--rtps-udp-transport-configuration-options` as the parameters for configuring an RTPS transport also apply to SEDP.

The following are additional implementation limits that developers need to take into consideration when developing and deploying applications that use RTPS discovery:

#. Domain IDs should be between 0 and 231 (inclusive) due to the way UDP ports are assigned to domain IDs.
   In each OpenDDS process, up to 120 domain participants are supported in each domain.

#. Topic names and type identifiers are limited to 256 characters.

#. OpenDDS's native multicast transport does not work with RTPS Discovery due to the way GUIDs are assigned (a warning will be issued if this is attempted).

.. _introduction--static-discovery:

Static Discovery
----------------

In Static Discovery, each particpant starts with a database containing identifiers, QoS settings, and network locators for all participants, topics, data writers, data readers.
The RTPS transport must be used with Static Discovery.
When an application creates a data writer or data reader, Static Discovery causes it to send out periodic announcements.
Upon receiving one of these announcements, Static Discovery consults its local database of entities to look up the details necessary for matching and matches it against local entities.
See :ref:`run_time_configuration--configuring-for-static-discovery` for details on how applications can be configured to use Static Discovery.

Static Discovery requires that the :ref:`quality_of_service--user-data` QoS be configured for each participant, data writer, and data reader.
This user data must contain the identifier of the entity that is being created.
Thus, the user data QoS is not available for general use when using Static Discovery.
Static Discovery also requires that the network locators for all entities be determined up front by configuring the transport with the necessary networking information.

.. _introduction--threading:

Threading
=========

..
    Sect<1.2.3.4>

OpenDDS creates its own threads for handling I/O, timers, asynchronous jobs, and cleanup tasks.
These threads are collectively called *service threads*.
Applications may receive a callback from these threads via :ref:`introduction--listeners` (see :ref:`conditions_and_listeners--listeners`).

When publishing a sample, OpenDDS normally attempts to send the sample to any connected subscribers using the calling thread.
If the send call would block, then the sample may be queued for sending on a separate service thread.
This behavior depends on the QoS policies described in :ref:`qos`.

All incoming data is read by a service thread and queued for reading in DataReaders by the application.
If a DataReader has a listener that should be invoked when data is available, then the listener is invoked by the service thread.

.. _introduction--configuration:

Configuration
=============

..
    Sect<1.2.3.5>

OpenDDS includes a file-based configuration framework for configuring both global items such as debug level, memory allocation, and discovery, as well as transport implementation details for publishers and subscribers.
Configuration can also be achieved directly in code, however, it is recommended that configuration be externalized for ease of maintenance and reduction in runtime errors.
The complete set of configuration options are described in :ref:`config`.
