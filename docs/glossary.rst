########
Glossary
########

************
Common Terms
************

..
  NOTE: This list isn't meant to be an all-encompassing dictionary. It's just
  meant to cover the most common terms a C++ developer who is new to OpenDDS
  might want to be familar with to use it. Parts of the API outside of the most
  important classes shouldn't be included. Definitions should be a few
  sentences at most and should link elsewhere for more information.

  TODO: Link to a API reference when available

.. glossary::
  :sorted:

  Adaptive Communication Environment
  ACE
    ACE is an open source C++ framework that is maintained alongside :term:`TAO` and is extensively used by OpenDDS.
    It provides a platform abstraction layer and various utilities such as reactors for event handing.

    See :ref:`deps-ace` for more information.

  Built-in Topics
  BITs
    Built-in Topics are :term:`topics <Topic>` that contain meta-information about local and remote :term:`DDS entites <Entity>` and the operational status of OpenDDS.

    See :ref:`bit` for more information.

  Common Data Representation
  CDR
  Extended Common Data Representation
  XCDR
    CDR is a family of binary encoding formats used by OpenDDS to serialize and deserialize :term:`samples <Sample>`.
    XCDR is an extended form of CDR defined by the :term:`XTypes` specification.

    See :ref:`xtypes--data-representation` for more information.

  Common Object Request Broker Architecture
  CORBA
    CORBA, put simply, is a set of specifications that allows a client to perform remote procedure calls on objects held in a server.
    It is separate from :term:`DDS`, but both are published by the :term:`OMG` and make use of :term:`IDL` and :term:`CDR`.
    :term:`TAO` is a CORBA implementation that OpenDDS uses for InfoRepo discovery.

    See https://www.corba.org/ for more information.

  Condition
    Conditions are a method of being notified of events from :term:`entities <Entity>`, such a new :term:`sample` being available from a :term:`DataReader`, via a status that can be checked synchronously.

    See :doc:`devguide/conditions_and_listeners` for more information.

  Data-Centric Publish-Subscribe
  DCPS
    DCPS is essentially synonymous with :term:`DDS`, but can specifically refer to the API described in the main DDS specification.

  Data Distribution Service
  DDS
    DDS is a specification for exchanging strongly-typed data across a distributed system.
    It is essentially synonymous with :term:`DCPS`, but can specifically refer to the main DDS specification.

    See :ref:`spec-dds` for more information.

  ``DataReader``
    A ``DataReader`` is an :term:`Entity` that is used to read :term:`samples <Sample>` from a :term:`topic`.
    ``DataReader``\s are created and managed by :term:`Subscribers <Subscriber>`.

  ``DataWriter``
    A ``DataWriter`` is an :term:`Entity` that is used to write :term:`samples <Sample>` to a :term:`topic`.
    ``DataWriter``\s are created and managed by :term:`Publishers <Publisher>`.

  Discovery
    Discovery is the configurable method that :term:`DomainParticipant`\s use to find one another.

    See :ref:`dds-introduction--discovery` for more information.

  Dispose
    When an :term:`instance` is disposed by a :term:`DataWriter`, it means keeping the cached data related to it is no longer necessary.
    This is done automatically if the instance is :term:`unregister`\ed and ``autodispose_unregistered_instances`` of :ref:`qos-writer-data-lifecycle` is set to ``true``.
    This can also be done manually using ``dispose`` method on ``DataWriter``.

  Domain
    Domains are sets of :term:`DomainParticipant` that can interact with one another.
    To interact they must share the same domain identifier and must have compatible :term:`discovery` and :term:`transport`.

  ``DomainParticipant``
    ``DomainParticipant`` is an :term:`Entity` that is used to create and manage :term:`Publishers <Publisher>` and :term:`Subscribers <Subscriber>`.

  ``Entity``
    ``Entity`` is the abstract base class for the main classes in the :term:`DDS`/:term:`DCPS` API, including :term:`DataReader` and :term:`DataWriter`.
    All entities have :term:`QoS` and can accept :term:`Listener`\s and :term:`Condition`\s.

  Instance
    In the specific context of :term:`DDS` and :term:`sample`\s, instances refer to a set of samples that share a common key value.
    Many parts of the DDS API involve instances, including the :term:`dispose` and :term:`unregister` operations.

    See :ref:`getting_started--keys` for more information.

  Interface Definition Language
  IDL
    IDL, specifically :term:`OMG` IDL, is a C-like language for describing data-structures and interfaces.
    In OpenDDS :term:`opendds_idl` and :term:`tao_idl` are used to generate code from IDL.

  Listener
    Listeners are a method of being notified of events from :term:`entities <Entity>`, such a new :term:`sample` being available from a :term:`DataReader`, via asynchronous callbacks.

    See :doc:`devguide/conditions_and_listeners` for more information.

  Makefile, Project, and Workspace Creator
  MPC
  ``mwc.pl``
  ``mpc.pl``
    A build-system that generates GNU Makefiles, Visual Studio projects, and other such files.
    It serves the same role that CMake, Meson, and Automake do in other projects.

    See :ref:`deps-mpc` for more information.

  Object Management Group
  OMG
    A standards organization which publishes :ref:`DDS and the other specifications used by OpenDDS <specs>`.

    See https://www.omg.org/ for more information.

  ``opendds_idl``
    A program that generates C++ code from :term:`IDL` for use in OpenDDS.

  ``Publisher``
    ``Publisher`` is an :term:`Entity` that is used to create and manage :term:`DataWriters <DataWriter>`.

  Quality of Service
  QoS
    QoS is a set of requested policies for how :term:`entities <Entity>` should behave.

    See :doc:`devguide/quality_of_service` for more information.

  Real-time Publish-Subscribe
  RTPS
    RTPS, sometimes also called *DDSI-RTPS*, is a specification that defines how different DDS implementations can interact with one another.

    See :ref:`spec-rtps` for more information.

  Sample
    Samples are the messages sent from :term:`DataWriter`\s and received by :term:`DataReader`\s.

  ``Subscriber``
    ``Subscriber`` is an :term:`Entity` that is used to create and manage :term:`DataReaders <DataReader>`.

  The ACE ORB
  TAO
    TAO is a :term:`CORBA` implementation that is maintained alongside :term:`ACE`.
    OpenDDS uses it for InfoRepo discovery and :term:`tao_idl`.

    See :ref:`deps-tao` for more information.

  ``tao_idl``
    A program that is part of :term:`TAO` that generates C++ code from :term:`IDL` for use in TAO and OpenDDS.

  ``Topic``
    A ``Topic`` is an :term:`Entity` with a name and a :term:`type <Topic type>` that the system uses to figure out which :term:`DataReader`\s get a :term:`sample` from a :term:`DataWriter`.

  Topic type
    A topic type, sometimes also called a *data type*, is the :term:`IDL` type of a :term:`topic` and also type of the :term:`samples <Sample>` of the :term:`DataWriter`\s and :term:`DataReader`\s that use that topic.

  Transport
    Transports are the configurable methods that :term:`DataWriter`\s and :term:`DataReader`\s use to communicate.

  Unregister
    When an :term:`instance` is unregistered by a :term:`DataWriter`, it means the writer "no longer has ‘anything to say’ about the instance", as phrased by the DDS specification.
    This is similar, but separate from :term:`disposing <dispose>` an instance.
    They are usually done at the same time, but this can be changed using :ref:`qos-writer-data-lifecycle` and the ``unregister_instance`` method of ``DataWriter`` to make what unregister means application-defined.

  XTypes
  Extensible and Dynamic Topic Types for DDS
    XTypes is an :term:`OMG` specification that defines how DDS systems can have :term:`topic type`\s that can evolve over time and be used without defining IDL.

    See :ref:`spec-xtypes` and :doc:`devguide/xtypes` for more information.

*********************
Environment Variables
*********************

.. envvar:: ACE_ROOT

  The path of the :term:`ACE` source tree or installation prefix being used.

.. envvar:: DDS_ROOT

  The path of the OpenDDS source tree or installation prefix being used.

.. envvar:: TAO_ROOT

  The path of the :term:`TAO` source tree or installation prefix being used.

.. envvar:: OPENDDS_CONFIG_DIR

  Makes ``-DCPSConfigFile`` paths relative to the path in the environment variable.

  See :ref:`here <OPENDDS_CONFIG_DIR>` for more information.
