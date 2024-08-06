.. _alternate_interfaces_to_data:
.. _altdata:

############################
Alternate Interfaces to Data
############################

..
    Sect<12>

The DDS-DCPS approach to data transfer using synchronization of strongly-typed caches (DataWriter and DataReader) is not appropriate for all applications.
Therefore OpenDDS provides two different alternate interface approaches which are described in this section.
These are not defined by OMG specifications and may change in future releases of OpenDDS, including minor updates.
The two approaches are:

* Recorder and Replayer

  * These interfaces allow the application to create untyped stand-ins for DataReaders and/or DataWriters

  * Recorder can be used with the Dynamic Language Binding XTypes features (:ref:`xtypes--dynamic-language-binding-1`) to access typed data samples through a reflection-based API

* Observer

  * Observers play a role similar to the spec-defined Listeners (attached to DataReaders and/or DataWriters).
    Unlike the Listeners, Observers don't need to interact with the DataReader/Writer caches to access the data samples.

The XTypes Dynamic Language Binding (:ref:`xtypes--dynamic-language-binding`) provides a set of related features that can be used to create DataWriters and DataReaders that work with a generic data container (DynamicData) instead of a specific IDL-generated data type.

.. _alternate_interfaces_to_data--recorder-and-replayer:

*********************
Recorder and Replayer
*********************

..
    Sect<12.1>

The Recorder feature of OpenDDS allows applications to record samples published on arbitrary topics without any prior knowledge of the data type used by that topic.
Analogously, the Replayer feature allows these recorded samples to by re-published back into the same or other topics.
What makes these features different from other Data Readers and Writers are their ability to work with any data type, even if unknown at application build time.
Effectively, the samples are treated as if each one contains an opaque byte sequence.

The purpose of this section is to describe the public API for OpenDDS to enable the recording/replaying use-case.

.. _alternate_interfaces_to_data--api-structure:

API Structure
=============

..
    Sect<12.1.1>

Two new user-visible classes (that behave somewhat like their DDS Entity counterparts) are defined in the ``OpenDDS::DCPS`` namespace, along with the associated Listener interfaces.
Listeners may be optionally implemented by the application.
The ``Recorder`` class acts similarly to a ``DataReader`` and the ``Replayer`` class acts similarly to a ``DataWriter``.

Both ``Recorder`` and ``Replayer`` make use of the underlying OpenDDS discovery and transport libraries as if they were ``DataReader`` and ``DataWriter``, respectively.
Regular OpenDDS applications in the domain will "see" the ``Recorder`` objects as if they were remote ``DataReader``\s and ``Replayers`` as if they were ``DataWriter``\s.

.. _alternate_interfaces_to_data--usage-model:

Usage Model
===========

..
    Sect<12.1.2>

The application creates any number of ``Recorder``\s and ``Replayer``\s as necessary.
This could be based on using the :term:`built-in topics` to dynamically discover which topics are active in the Domain.
Creating a ``Recorder`` or ``Replayer`` requires the application to provide a topic name and type name (as in ``DomainParticipant::create_topic()``) and also the relevant QoS data structures.
The ``Recorder`` requires SubscriberQos and DataReaderQos whereas the ``Replayer`` requires PublisherQos and DataWriterQos.
These values are used in discovery's reader/writer matching.
See the section on QoS processing below for how the ``Recorder`` and ``Replayer`` use QoS.
Here is the code needed to create a recorder:

.. code-block:: cpp

    OpenDDS::DCPS::Recorder_var recorder =
         service_participant->create_recorder(domain_participant,
                                              topic.in(),
                                              sub_qos,
                                              dr_qos,
                                              recorder_listener);

Data samples are made available to the application via the ``RecorderListener`` using a simple "one callback per sample" model.
The sample is delivered as an ``OpenDDS::DCPS::RawDataSample`` object.
This object includes the timestamp for that data sample as well as the marshaled sample value.
Here is a class definition for a user-defined Recorder Listener.

.. code-block:: cpp

    class MessengerRecorderListener : public OpenDDS::DCPS::RecorderListener
    {
    public:
      MessengerRecorderListener();

      virtual void on_sample_data_received(OpenDDS::DCPS::Recorder*,
                                           const OpenDDS::DCPS::RawDataSample& sample);

      virtual void on_recorder_matched(OpenDDS::DCPS::Recorder*,
                                       const DDS::SubscriptionMatchedStatus& status );

    };

The application can store the data wherever it sees fit (in memory, file system, database, etc.).
At any later time, the application can provide that same sample to a ``Replayer`` object configured for the same topic.
It's the application's responsibility to make sure the topic types match.
Here is an example call that replays a sample to all readers connected on a replayer's topic:

.. code-block:: cpp

         replayer->write(sample);

Because the stored data is dependent on the definition of the data structure, it can't be used across different versions of OpenDDS or different versions of the IDL used by the OpenDDS participants.

.. _alternate_interfaces_to_data--qos-processing:

QoS Processing
==============

..
    Sect<12.1.3>

The lack of detailed knowledge about the data sample complicates the use of many normal DDS QoS properties on the ``Replayer`` side.
The properties can be divided into a few categories:

* Supported

  * Liveliness
  * Time-Based Filter
  * Lifespan
  * Durability (transient local level, see details below)
  * Presentation (topic level only)
  * Transport Priority (pass-thru to transport)

* Unsupported

  * Deadline (still used for reader/writer match)
  * History
  * Resource Limits
  * Durability Service
  * Ownership and Ownership Strength (still used for reader/writer match)

* Affects reader/writer matching and :term:`built-in topics` but otherwise ignored

  * Partition
  * Reliability (still used by transport negotiation)
  * Destination Order
  * Latency Budget
  * User/Group Data

.. _alternate_interfaces_to_data--durability-details:

Durability details
------------------

..
    Sect<12.1.3.1>

On the ``Recorder`` side, transient local durability works just the same as any normal ``DataReader``.
Durable data is received from matched ``DataWriter``\s.
On the ``Replayer``\side there are some differences.
As opposed to the normal DDS ``DataWriter``, ``Replayer`` is not caching/storing any data samples (they are simply sent to the transport).
Because instances are not known, storing data samples according to the usual History and Resource Limits rules is not possible.
Instead, transient local durability can be supported with a "pull" model whereby the middleware invokes a method on the ``ReplayerListener`` when a new remote ``DataReader`` is discovered.
The application can then call a method on the ``Replayer`` with any data samples that should be sent to that newly-joined ``DataReader``.
Determining which samples these are is left to the application.

.. _alternate_interfaces_to_data--recorder-with-xtypes-dynamic-language-binding:

Recorder With XTypes Dynamic Language Binding
=============================================

..
    Sect<12.1.4>

The Recorder class includes support for the Dynamic Language Binding from XTypes (:ref:`xtypes--dynamic-language-binding-1`).
Type information for each matched DataWriter (that supports XTypes complete TypeObjects) is stored in the Recorder.
Users can call ``Recorder::get_dynamic_data``, passing a ``RawDataSample`` to get back a ``DynamicData`` object which includes type information -- see ``DynamicData::type()``.

A tool called ``inspect``, uses the Recorder and Dynamic Language Binding allow for the printing of any type, so long as the topic name, type name, and domain ID are known.
The DataWriter must include code generation for complete TypeObjects.
See :ghfile:`tools/inspect/Inspect.cpp` for this tool's source code.
It can be used as a standalone tool or an example for developing your own applications using these APIs.

.. _alternate_interfaces_to_data--observer:

********
Observer
********

..
    Sect<12.2>

To observe the most important events happening within OpenDDS, applications can create classes that derive from the Observer base class (in :ghfile:`dds/DCPS/Observer.h`).
The design of Observer is intended to allow applications to have a single Observer object observing many Entities, however this is flexible to allow many different use cases.
The following events can be observed:

* DataWriter/Reader enabled, deleted

* DataWriter/Reader QoS changed

* DataWriter/Reader peer associated, disassociated

* DataWriter sample sent, instance disposed, instance unregistered

* DataReader sample received (enters the cache), read, taken, instance disposed, instance unregistered

.. _alternate_interfaces_to_data--attaching-observers-to-entities:

Attaching Observers to Entities
===============================

..
    Sect<12.2.1>

Entity is the spec-defined base interface of the following types:

* DataWriter, DataReader

  * As seen above in :ref:`alternate_interfaces_to_data--observer`, the Observer events originate in the DataWriter and DataReader Entities

* DomainParticipant, Publisher, Subscriber

  * Among their other roles, these Entities act as containers (either directly or indirectly) for DataWriters and DataReaders.

  * If a smaller-scoped Entity (such as a DataWriter) has no Observer for the event in question, its containing Entity (in this example, a Publisher) is checked for an Observer.

* Topic

  * Although it is an Entity, no Observer events are generated by Topics or Entities they contain (since they don't contain any Entities)

The class ``EntityImpl`` (in :ghfile:`dds/DCPS/EntityImpl.h`) is OpenDDS's base class for all Entity types.
``EntityImpl`` includes public methods for Observer registration: ``set_observer`` and ``get_observer``.
These methods are not part of the IDL interfaces, so invoking them the requires a cast to the implementation (Impl) of Entity.

.. code-block:: cpp

     DDS::DataWriter_var dw = /* ... */;
     EntityImpl* entity = dynamic_cast<EntityImpl*>(dw.in());
     Observer_rch observer = make_rch<MyObserver>();
     entity->set_observer(observer, Observer::e_SAMPLE_SENT);

Note that since the ``Observer`` class is an internal (not IDL) interface, it uses the "RCH" (Reference Counted Handle) smart pointer classes.
Observer itself inherits from ``RcObject``, and uses of ``Observer``-derived classes should use the ``RcHandle`` template and its associated functions, as in the example above.
See :ghfile:`dds/DCPS/RcHandle_T.h` for details.

.. _alternate_interfaces_to_data--writing-observer-derived-classes:

Writing Observer-Derived Classes
================================

..
    Sect<12.2.2>

The virtual methods in the ``Observer`` class are divided into 3 groups based on the general category of events they observe:

#. Operations on the observed ``Entity`` itself

   * ``on_enabled``, ``on_deleted``, ``on_qos_changed``

   * The only parameter to these methods is the ``Entity``, so the ``Observer`` implementation can use the public methods on the ``Entity``.

#. Events relating to associating with remote matched endpoints

   * ``on_associated``, ``on_disassociated``

   * In addition to the ``Entity``, the ``Observer`` implementation receives a ``GUID_t`` structure which is the internal representation of remote ``Entity`` identity.
     The ``GUID_t`` values from ``on_associated`` could be stored or logged to correlate them with the values from ``on_disassociated``.

#. Events relating to data samples moving through the system

   * ``on_sample_sent``, ``on_sample_received``, ``on_sample_read``, ``on_sample_taken``, ``on_disposed``, ``on_unregistered``

   * In addition to the ``Entity``, the ``Observer`` implementation receives an instance of the ``Sample`` structure.
     The definition of this structure is nested within ``Observer``.
     See below for details.

.. _alternate_interfaces_to_data--the-observer-sample-structure:

The Observer::Sample structure
==============================

..
    Sect<12.2.3>

The ``Observer::Sample`` structure contains the following fields:

* ``instance`` and ``instance_state``

  * Describe the instance that this sample belongs to, using the spec-defined types

* ``timestamp`` and ``sequence_number``

  * Attributes of the sample itself: ``timestamp`` uses a spec-defined type whereas ``sequence_number`` uses the OpenDDS internal type for DDSI-RTPS 64-bit sequence numbers.

* ``data`` and ``data_dispatcher``

  * Since ``Observer`` is an un-typed interface, the contents of the data sample itself are represented only as a void pointer

  * Implementations that need to process this data can use the ``data_dispatcher`` object to interpret it.
    See the class definition of ``ValueDispatcher`` in :ghfile:`dds/DCPS/ValueDispatcher.h` for more details.

