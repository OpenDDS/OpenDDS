.. _quality_of_service--quality-of-service:

##################
Quality of Service
##################

..
    Sect<3>

.. _quality_of_service--introduction:

************
Introduction
************

..
    Sect<3.1>

The previous examples use default QoS policies for the various entities.
This chapter discusses the QoS policies which are implemented in OpenDDS and the details of their usage.
See the DDS specification for further information about the policies discussed in this chapter.

.. _quality_of_service--qos-policies:

************
QoS Policies
************

..
    Sect<3.2>

Each policy defines a structure to specify its data.
Each entity supports a subset of the policies and defines a QoS structure that is composed of the supported policy structures.
The set of allowable policies for a given entity is constrained by the policy structures nested in its QoS structure.
For example, the Publisher’s QoS structure is defined in the specification’s IDL as follows:

.. code-block:: omg-idl

    module DDS {
      struct PublisherQos {
       PresentationQosPolicy presentation;
       PartitionQosPolicy partition;
       GroupDataQosPolicy group_data;
       EntityFactoryQosPolicy entity_factory;
     };
    };

Setting policies is as simple as obtaining a structure with the default values already set, modifying the individual policy structures as necessary, and then applying the QoS structure to an entity (usually when it is created).
We show examples of how to obtain the default QoS policies for various entity types in Section :ref:`quality_of_service--default-qos-policy-values`.

Applications can change the QoS of any entity by calling the set_qos() operation on the entity.
If the QoS is changeable, existing associations are removed if they are no longer compatible and new associations are added if they become compatible.
The ``DCPSInfoRepo`` re-evaluates the QoS compatibility and associations according to the QoS specification.
If the compatibility checking fails, the call to set_qos() will return an error.
The association re-evaluation may result in removal of existing associations or addition of new associations.

If the user attempts to change a QoS policy that is immutable (not changeable), then ``set_qos()`` returns ``DDS::RETCODE_IMMUTABLE_POLICY``.

A subset of the QoS policies are changeable.
Some changeable QoS policies, such as ``USER_DATA``, ``TOPIC_DATA``, ``GROUP_DATA``, ``LIFESPAN``, ``OWNERSHIP_STRENGTH``, ``TIME_BASED_FILTER``, ``ENTITY_FACTORY``, ``WRITER_DATA_LIFECYCLE``, and ``READER_DATA_LIFECYCLE``, do not require compatibility and association re-evaluation.
The ``DEADLINE`` and ``LATENCY_BUDGET`` QoS policies require compatibility re-evaluation, but not for association.
The ``PARTITION`` QoS policy does not require compatibility re-evaluation, but does require association re-evaluation.
The DDS specification lists ``TRANSPORT_PRIORITY`` as changeable, but the OpenDDS implementation does not support dynamically modifying this policy.

.. _quality_of_service--default-qos-policy-values:

Default QoS Policy Values
=========================

..
    Sect<3.2.1>

Applications obtain the default QoS policies for an entity by instantiating a QoS structure of the appropriate type for the entity and passing it by reference to the appropriate ``get_default_entity_qos()`` operation on the appropriate factory entity.
(For example, you would use a domain participant to obtain the default QoS for a publisher or subscriber.)
The following examples illustrate how to obtain the default policies for publisher, subscriber, topic, domain participant, data writer, and data reader.

.. code-block:: cpp

    // Get default Publisher QoS from a DomainParticipant:
    DDS::PublisherQos pub_qos;
    DDS::ReturnCode_t ret;
    ret = domain_participant->get_default_publisher_qos(pub_qos);
    if (DDS::RETCODE_OK != ret) {
      std::cerr << "Could not get default publisher QoS" << std::endl;
    }

    // Get default Subscriber QoS from a DomainParticipant:
    DDS::SubscriberQos sub_qos;
    ret = domain_participant->get_default_subscriber_qos(sub_qos);
    if (DDS::RETCODE_OK != ret) {
      std::cerr << "Could not get default subscriber QoS" << std::endl;
    }

    // Get default Topic QoS from a DomainParticipant:
    DDS::TopicQos topic_qos;
    ret = domain_participant->get_default_topic_qos(topic_qos);
    if (DDS::RETCODE_OK != ret) {
      std::cerr << "Could not get default topic QoS" << std::endl;
    }

    // Get default DomainParticipant QoS from a DomainParticipantFactory:
    DDS::DomainParticipantQos dp_qos;
    ret = domain_participant_factory->get_default_participant_qos(dp_qos);
    if (DDS::RETCODE_OK != ret) {
      std::cerr << "Could not get default participant QoS" << std::endl;
    }

    // Get default DataWriter QoS from a Publisher:
    DDS::DataWriterQos dw_qos;
    ret = pub->get_default_datawriter_qos(dw_qos);
    if (DDS::RETCODE_OK != ret) {
      std::cerr << "Could not get default data writer QoS" << std::endl;
    }

    // Get default DataReader QoS from a Subscriber:
    DDS::DataReaderQos dr_qos;
    ret = sub->get_default_datareader_qos(dr_qos);
    if (DDS::RETCODE_OK != ret) {
      std::cerr << "Could not get default data reader QoS" << std::endl;
    }

The following tables summarize the default QoS policies for each entity type in OpenDDS to which policies can be applied.

.. _quality_of_service--reftable2:

**Table  Default DomainParticipant QoS Policies**

.. list-table::
   :header-rows: 1

   * - Policy

     - Member

     - Default Value

   * - ``USER_DATA``

     - ``value``

     - ``(empty sequence)``

   * - ``ENTITY_FACTORY``

     - ``autoenable_created_entities``

     - ``true``

.. _quality_of_service--reftable3:

**Table  Default Topic QoS Policies**

.. list-table::
   :header-rows: 1

   * - Policy

     - Member

     - Default Value

   * - ``TOPIC_DATA``

     - ``value``

     - ``(empty sequence)``

   * - ``DURABILITY``

     - ``kind``

       ``service_cleanup_delay.sec``

       ``service_cleanup_delay.nanosec``

     - ``VOLATILE_DURABILITY_QOS``

       ``DURATION_ZERO_SEC``

       ``DURATION_ZERO_NSEC``

   * - ``DURABILITY_SERVICE``

     - ``service_cleanup_delay.sec``

       ``service_cleanup_delay.nanosec``

       ``history_kind``

       ``history_depth``

       ``max_samples``

       ``max_instances``

       ``max_samples_per_instance``

     - ``DURATION_ZERO_SEC``

       ``DURATION_ZERO_NSEC``

       ``KEEP_LAST_HISTORY_QOS``

       ``1``

       ``LENGTH_UNLIMITED``

       ``LENGTH_UNLIMITED``

       ``LENGTH_UNLIMITED``

   * - ``DEADLINE``

     - ``period.sec``

       ``period.nanosec``

     - ``DURATION_INFINITE_SEC``

       ``DURATION_INFINITE_NSEC``

   * - ``LATENCY_BUDGET``

     - ``duration.sec``

       ``duration.nanosec``

     - ``DURATION_ZERO_SEC``

       ``DURATION_ZERO_NSEC``

   * - ``LIVELINESS``

     - ``kind``

       ``lease_duration.sec``

       ``lease_duration.nanosec``

     - ``AUTOMATIC_LIVELINESS_QOS``

       ``DURATION_INFINITE_SEC``

       ``DURATION_INFINITE_NSEC``

   * - ``RELIABILITY``

     - ``kind``

       ``max_blocking_time.sec``

       ``max_blocking_time.nanosec``

     - ``BEST_EFFORT_RELIABILITY_QOS``

       ``DURATION_INFINITE_SEC``

       ``DURATION_INFINITE_NSEC``

   * - ``DESTINATION_ORDER``

     - ``kind``

     - ``BY_RECEPTION_TIMESTAMP_``

       ``DESTINATIONORDER_QOS``

   * - ``HISTORY``

     - ``kind``

       ``depth``

     - ``KEEP_LAST_HISTORY_QOS``

       ``1``

   * - ``RESOURCE_LIMITS``

     - ``max_samples``

       ``max_instances``

       ``max_samples_per_instance``

     - ``LENGTH_UNLIMITED``

       ``LENGTH_UNLIMITED``

       ``LENGTH_UNLIMITED``

   * - ``TRANSPORT_PRIORITY``

     - ``value``

     - ``0``

   * - ``LIFESPAN``

     - ``duration.sec``

       ``duration.nanosec``

     - ``DURATION_INFINITE_SEC``

       ``DURATION_INFINITE_NSEC``

   * - ``OWNERSHIP``

     - ``kind``

     - ``SHARED_OWNERSHIP_QOS``

.. _quality_of_service--reftable4:

**Table  Default Publisher QoS Policies**

.. list-table::
   :header-rows: 1

   * - Policy

     - Member

     - Default Value

   * - ``PRESENTATION``

     - ``access_scope``

       ``coherent_access``

       ``ordered_access``

     - ``INSTANCE_PRESENTATION_QOS``

       ``0``

       ``0``

   * - ``PARTITION``

     - ``name``

     - ``(empty sequence)``

   * - ``GROUP_DATA``

     - ``value``

     - ``(empty sequence)``

   * - ``ENTITY_FACTORY``

     - ``autoenable_created_entities``

     - ``true``

.. _quality_of_service--reftable5:

**Table  Default Subscriber QoS Policies**

.. list-table::
   :header-rows: 1

   * - Policy

     - Member

     - Default Value

   * - ``PRESENTATION``

     - ``access_scope``

       ``coherent_access``

       ``ordered_access``

     - ``INSTANCE_PRESENTATION_QOS``

       ``0``

       ``0``

   * - ``PARTITION``

     - ``name``

     - ``(empty sequence)``

   * - ``GROUP_DATA``

     - ``value``

     - ``(empty sequence)``

   * - ``ENTITY_FACTORY``

     - ``autoenable_created_entities``

     - ``true``

.. _quality_of_service--reftable6:

**Table  Default DataWriter QoS Policies**

.. list-table::
   :header-rows: 1

   * - Policy

     - Member

     - Default Value

   * - ``DURABILITY``

     - ``kind``

       ``service_cleanup_delay.sec``

       ``service_cleanup_delay.nanosec``

     - ``VOLATILE_DURABILITY_QOS``

       ``DURATION_ZERO_SEC``

       ``DURATION_ZERO_NSEC``

   * - ``DURABILITY_SERVICE``

     - ``service_cleanup_delay.sec``

       ``service_cleanup_delay.nanosec``

       ``history_kind``

       ``history_depth``

       ``max_samples``

       ``max_instances``

       ``max_samples_per_instance``

     - ``DURATION_ZERO_SEC``

       ``DURATION_ZERO_NSEC``

       ``KEEP_LAST_HISTORY_QOS``

       ``1``

       ``LENGTH_UNLIMITED``

       ``LENGTH_UNLIMITED``

       ``LENGTH_UNLIMITED``

   * - ``DEADLINE``

     - ``period.sec``

       ``period.nanosec``

     - ``DURATION_INFINITE_SEC``

       ``DURATION_INFINITE_NSEC``

   * - ``LATENCY_BUDGET``

     - ``duration.sec``

       ``duration.nanosec``

     - ``DURATION_ZERO_SEC``

       ``DURATION_ZERO_NSEC``

   * - ``LIVELINESS``

     - ``kind``

       ``lease_duration.sec``

       ``lease_duration.nanosec``

     - ``AUTOMATIC_LIVELINESS_QOS``

       ``DURATION_INFINITE_SEC``

       ``DURATION_INFINITE_NSEC``

   * - ``RELIABILITY``

     - ``kind``

       ``max_blocking_time.sec``

       ``max_blocking_time.nanosec``

     - ``RELIABLE_RELIABILITY_QOS`` [#footnote1]_

       ``0``

       ``100000000 (100 ms)``

   * - ``DESTINATION_ORDER``

     - ``kind``

     - ``BY_RECEPTION_TIMESTAMP_``

       ``DESTINATIONORDER_QOS``

   * - ``HISTORY``

     - ``kind``

       ``depth``

     - ``KEEP_LAST_HISTORY_QOS``

       ``1``

   * - ``RESOURCE_LIMITS``

     - ``max_samples``

       ``max_instances``

       ``max_samples_per_instance``

     - ``LENGTH_UNLIMITED``

       ``LENGTH_UNLIMITED``

       ``LENGTH_UNLIMITED``

   * - ``TRANSPORT_PRIORITY``

     - ``value``

     - ``0``

   * - ``LIFESPAN``

     - ``duration.sec``

       ``duration.nanosec``

     - ``DURATION_INFINITE_SEC``

       ``DURATION_INFINITE_NSEC``

   * - ``USER_DATA``

     - ``value``

     - ``(empty sequence)``

   * - ``OWNERSHIP``

     - ``kind``

     - ``SHARED_OWNERSHIP_QOS``

   * - ``OWNERSHIP_STRENGTH``

     - ``value``

     - ``0``

   * - ``WRITER_DATA_LIFECYCLE``

     - ``autodispose_unregistered_instances``

     - ``1``

.. _quality_of_service--reftable7:

**Table  Default DataReader QoS Policies**

.. list-table::
   :header-rows: 1

   * - Policy

     - Member

     - Default Value

   * - ``DURABILITY``

     - ``kind``

       ``service_cleanup_delay.sec``

       ``service_cleanup_delay.nanosec``

     - ``VOLATILE_DURABILITY_QOS``

       ``DURATION_ZERO_SEC``

       ``DURATION_ZERO_NSEC``

   * - ``DEADLINE``

     - ``period.sec``

       ``period.nanosec``

     - ``DURATION_INFINITE_SEC``

       ``DURATION_INFINITE_NSEC``

   * - ``LATENCY_BUDGET``

     - ``duration.sec``

       ``duration.nanosec``

     - ``DURATION_ZERO_SEC``

       ``DURATION_ZERO_NSEC``

   * - ``LIVELINESS``

     - ``kind``

       ``lease_duration.sec``

       ``lease_duration.nanosec``

     - ``AUTOMATIC_LIVELINESS_QOS``

       ``DURATION_INFINITE_SEC``

       ``DURATION_INFINITE_NSEC``

   * - ``RELIABILITY``

     - ``kind``

       ``max_blocking_time.sec``

       ``max_blocking_time.nanosec``

     - ``BEST_EFFORT_RELIABILITY_QOS``

       ``DURATION_INFINITE_SEC``

       ``DURATION_INFINITE_NSEC``

   * - ``DESTINATION_ORDER``

     - ``kind``

     - ``BY_RECEPTION_TIMESTAMP_``

       ``DESTINATIONORDER_QOS``

   * - ``HISTORY``

     - ``kind``

       ``depth``

     - ``KEEP_LAST_HISTORY_QOS``

       ``1``

   * - ``RESOURCE_LIMITS``

     - ``max_samples``

       ``max_instances``

       ``max_samples_per_instance``

     - ``LENGTH_UNLIMITED``

       ``LENGTH_UNLIMITED``

       ``LENGTH_UNLIMITED``

   * - ``USER_DATA``

     - ``value``

     - ``(empty sequence)``

   * - ``OWNERSHIP``

     - ``kind``

     - ``SHARED_OWNERSHIP_QOS``

   * - ``TIME_BASED_FILTER``

     - ``minimum_separation.sec``

       ``minimum_separation.nanosec``

     - ``DURATION_ZERO_SEC``

       ``DURATION_ZERO_NSEC``

   * - ``READER_DATA_LIFECYCLE``

     - ``autopurge_nowriter_samples_delay.sec``

       ``autopurge_nowriter_samples_delay.nanosec``

       ``autopurge_disposed_samples_delay.sec``

       ``autopurge_disposed_samples_delay.nanosec``

     - ``DURATION_INFINITE_SEC``

       ``DURATION_INFINITE_NSEC``

       ``DURATION_INFINITE_SEC``

       ``DURATION_INFINITE_NSEC``

.. _quality_of_service--liveliness:

LIVELINESS
==========

..
    Sect<3.2.2>

The ``LIVELINESS`` policy applies to the topic, data reader, and data writer entities via the liveliness member of their respective QoS structures.
Setting this policy on a topic means it is in effect for all data readers and data writers on that topic.
Below is the IDL related to the liveliness QoS policy:

.. code-block:: omg-idl

    enum LivelinessQosPolicyKind {
      AUTOMATIC_LIVELINESS_QOS,
      MANUAL_BY_PARTICIPANT_LIVELINESS_QOS,
      MANUAL_BY_TOPIC_LIVELINESS_QOS
    };

    struct LivelinessQosPolicy {
      LivelinessQosPolicyKind kind;
      Duration_t lease_duration;
    };

The ``LIVELINESS`` policy controls when and how the service determines whether participants are alive, meaning they are still reachable and active.
The kind member setting indicates whether liveliness is asserted automatically by the service or manually by the specified entity.
A setting of ``AUTOMATIC_LIVELINESS_QOS`` means that the service will send a liveliness indication if the participant has not sent any network traffic for the lease_duration.
The ``MANUAL_BY_PARTICIPANT_LIVELINESS_QOS`` or ``MANUAL_BY_TOPIC_LIVELINESS_QOS`` setting means the specified entity (data writer for the “by topic” setting or domain participant for the “by participant” setting) must either write a sample or manually assert its liveliness within a specified heartbeat interval.
The desired heartbeat interval is specified by the lease_duration member.
The default lease duration is a pre-defined infinite value, which disables any liveliness testing.

To manually assert liveliness without publishing a sample, the application must call the ``assert_liveliness()`` operation on the data writer (for the “by topic” setting) or on the domain participant (for the “by participant” setting) within the specified heartbeat interval.

Data writers specify (*offer*) their own liveliness criteria and data readers specify (*request*) the desired liveliness of their writers.
Writers that are not heard from within the lease duration (either by writing a sample or by asserting liveliness) cause a change in the ``LIVELINESS_CHANGED_STATUS`` communication status and notification to the application (e.g., by calling the data reader listener’s ``on_liveliness_changed()`` callback operation or by signaling any related wait sets).

This policy is considered during the establishment of associations between data writers and data readers.
The value of both sides of the association must be compatible in order for an association to be established.
Compatibility is determined by comparing the data reader’s requested liveliness with the data writer’s offered liveliness.
Both the kind of liveliness (automatic, manual by topic, manual by participant) and the value of the lease duration are considered in determining compatibility.
The writer’s offered kind of liveliness must be greater than or equal to the reader’s requested kind of liveliness.
The liveliness kind values are ordered as follows:

::

    MANUAL_BY_TOPIC_LIVELINESS_QOS >
    MANUAL_BY_PARTICIPANT_LIVELINESS_QOS >
    AUTOMATIC_LIVELINESS_QOS

In addition, the writer’s offered lease duration must be less than or equal to the reader’s requested lease duration.
Both of these conditions must be met for the offered and requested liveliness policy settings to be considered compatible and the association established.

.. _quality_of_service--reliability:

RELIABILITY
===========

..
    Sect<3.2.3>

The ``RELIABILITY`` policy applies to the topic, data reader, and data writer entities via the reliability member of their respective QoS structures.
Below is the IDL related to the reliability QoS policy:

.. code-block:: omg-idl

    enum ReliabilityQosPolicyKind {
      BEST_EFFORT_RELIABILITY_QOS,
      RELIABLE_RELIABILITY_QOS
    };

    struct ReliabilityQosPolicy {
      ReliabilityQosPolicyKind kind;
      Duration_t max_blocking_time;
    };

This policy controls how data readers and writers treat the data samples they process.
The “best effort” value (``BEST_EFFORT_RELIABILITY_QOS``) makes no promises as to the reliability of the samples and could be expected to drop samples under some circumstances.
The “reliable” value (``RELIABLE_RELIABILITY_QOS``) indicates that the service should eventually deliver all values to eligible data readers.

The ``max_blocking_time`` member of this policy is used when the history QoS policy is set to “keep all” and the writer is unable to proceed because of resource limits.
When this situation occurs and the writer blocks for more than the specified time, then the write fails with a timeout return code.
The default for this policy for data readers and topics is “best effort,” while the default value for data writers is “reliable.”

This policy is considered during the creation of associations between data writers and data readers.
The value of both sides of the association must be compatible in order for an association to be created.
The reliability kind of data writer must be greater than or equal to the value of data reader.

.. _quality_of_service--history:

HISTORY
=======

..
    Sect<3.2.4>

The ``HISTORY`` policy determines how samples are held in the data writer and data reader for a particular instance.
For data writers these values are held until the publisher retrieves them and successfully sends them to all connected subscribers.
For data readers these values are held until “taken” by the application.
This policy applies to the topic, data reader, and data writer entities via the history member of their respective QoS structures.
Below is the IDL related to the history QoS policy:

.. code-block:: omg-idl

    enum HistoryQosPolicyKind {
      KEEP_LAST_HISTORY_QOS,
      KEEP_ALL_HISTORY_QOS
    };

    struct HistoryQosPolicy {
      HistoryQosPolicyKind kind;
      long depth;
    };

The “keep all” value (``KEEP_ALL_HISTORY_QOS``) specifies that all possible samples for that instance should be kept.
When “keep all” is specified and the number of unread samples is equal to the “resource limits” field of ``max_samples_per_instance`` then any incoming samples are rejected.

The “keep last” value (``KEEP_LAST_HISTORY_QOS``) specifies that only the last ``depth`` values should be kept.
When a data writer contains depth samples of a given instance, a write of new samples for that instance are queued for delivery and the oldest unsent samples are discarded.
When a data reader contains depth samples of a given instance, any incoming samples for that instance are kept and the oldest samples are discarded.

This policy defaults to a “keep last” with a ``depth`` of one.

.. _quality_of_service--durability:

DURABILITY
==========

..
    Sect<3.2.5>

The ``DURABILITY`` policy controls whether data writers should maintain samples after they have been sent to known subscribers.
This policy applies to the topic, data reader, and data writer entities via the durability member of their respective QoS structures.
Below is the IDL related to the durability QoS policy:

.. code-block:: omg-idl

    enum DurabilityQosPolicyKind {
      VOLATILE_DURABILITY_QOS,         // Least Durability
      TRANSIENT_LOCAL_DURABILITY_QOS,
      TRANSIENT_DURABILITY_QOS,
      PERSISTENT_DURABILITY_QOS        // Greatest Durability
    };

    struct DurabilityQosPolicy {
      DurabilityQosPolicyKind kind;
    };

By default the kind is ``VOLATILE_DURABILITY_QOS``.

A durability kind of ``VOLATILE_DURABILITY_QOS`` means samples are discarded after being sent to all known subscribers.
As a side effect, subscribers cannot recover samples sent before they connect.

A durability kind of ``TRANSIENT_LOCAL_DURABILITY_QOS`` means that data readers that are associated/connected with a data writer will be sent all of the samples in the data writer’s history.

A durability kind of ``TRANSIENT_DURABILITY_QOS`` means that samples outlive a data writer and last as long as the process is alive.
The samples are kept in memory, but are not persisted to permanent storage.
A data reader subscribed to the same topic and partition within the same domain will be sent all of the cached samples that belong to the same topic/partition.

A durability kind of ``PERSISTENT_DURABILITY_QOS`` provides basically the same functionality as transient durability except the cached samples are persisted and will survive process destruction.

When transient or persistent durability is specified, the ``DURABILITY_SERVICE`` QoS policy specifies additional tuning parameters for the durability cache.

The durability policy is considered during the creation of associations between data writers and data readers.
The value of both sides of the association must be compatible in order for an association to be created.
The durability kind value of the data writer must be greater than or equal to the corresponding value of the data reader.
The durability kind values are ordered as follows:

::

    PERSISTENT_DURABILITY_QOS >
    TRANSIENT_DURABILITY_QOS >
    TRANSIENT_LOCAL_DURABILITY_QOS >
    VOLATILE_DURABILITY_QOS

.. _quality_of_service--durability-service:

DURABILITY_SERVICE
==================

..
    Sect<3.2.6>

The ``DURABILITY_SERVICE`` policy controls deletion of samples in ``TRANSIENT`` or ``PERSISTENT`` durability cache.
This policy applies to the topic and data writer entities via the durability_service member of their respective QoS structures and provides a way to specify ``HISTORY`` and ``RESOURCE_LIMITS`` for the sample cache.
Below is the IDL related to the durability service QoS policy:

.. code-block:: omg-idl

    struct DurabilityServiceQosPolicy {
      Duration_t              service_cleanup_delay;
      HistoryQosPolicyKind    history_kind;
      long                    history_depth;
      long                    max_samples;
      long                    max_instances;
      long                    max_samples_per_instance;
    };

The history and resource limits members are analogous to, although independent of, those found in the ``HISTORY`` and ``RESOURCE_LIMITS`` policies.
The ``service_cleanup_delay`` can be set to a desired value.
By default, it is set to zero, which means never clean up cached samples.

.. _quality_of_service--resource-limits:

RESOURCE_LIMITS
===============

..
    Sect<3.2.7>

The ``RESOURCE_LIMITS`` policy determines the amount of resources the service can consume in order to meet the requested QoS.
This policy applies to the topic, data reader, and data writer entities via the resource_limits member of their respective QoS structures.
Below is the IDL related to the resource limits QoS policy.

.. code-block:: omg-idl

    struct ResourceLimitsQosPolicy {
      long max_samples;
      long max_instances;
      long max_samples_per_instance;
    };

The ``max_samples`` member specifies the maximum number of samples a single data writer or data reader can manage across all of its instances.
The ``max_instances`` member specifies the maximum number of instances that a data writer or data reader can manage.
The ``max_samples_per_instance`` member specifies the maximum number of samples that can be managed for an individual instance in a single data writer or data reader.
The values of all these members default to unlimited (``DDS::LENGTH_UNLIMITED``).

Resources are used by the data writer to queue samples written to the data writer but not yet sent to all data readers because of backpressure from the transport.
Resources are used by the data reader to queue samples that have been received, but not yet read/taken from the data reader.

.. _quality_of_service--partition:

PARTITION
=========

..
    Sect<3.2.8>

The ``PARTITION`` QoS policy allows the creation of logical partitions within a domain.
It only allows data readers and data writers to be associated if they have matched partition strings.
This policy applies to the publisher and subscriber entities via the partition member of their respective QoS structures.
Below is the IDL related to the partition QoS policy.

.. code-block:: omg-idl

    struct PartitionQosPolicy {
      StringSeq name;
    };

The name member defaults to an empty sequence of strings.
The default partition name is an empty string and causes the entity to participate in the default partition.
The partition names may contain wildcard characters as defined by the POSIX ``fnmatch`` function (POSIX 1003.2-1992 section B.6).

The establishment of data reader and data writer associations depends on matching partition strings on the publication and subscription ends.
Failure to match partitions is not considered a failure and does not trigger any callbacks or set any status values.

The value of this policy may be changed at any time.
Changes to this policy may cause associations to be removed or added.

.. _quality_of_service--deadline:

DEADLINE
========

..
    Sect<3.2.9>

The ``DEADLINE`` QoS policy allows the application to detect when data is not written or read within a specified amount of time.
This policy applies to the topic, data writer, and data reader entities via the deadline member of their respective QoS structures.
Below is the IDL related to the deadline QoS policy.

.. code-block:: omg-idl

    struct DeadlineQosPolicy {
      Duration_t period;
    };

The default value of the ``period`` member is infinite, which requires no behavior.
When this policy is set to a finite value, then the data writer monitors the changes to data made by the application and indicates failure to honor the policy by setting the corresponding status condition and triggering the ``on_offered_deadline_missed()`` listener callback.
A data reader that detects that the data has not changed before the period has expired sets the corresponding status condition and triggers the ``on_requested_deadline_missed()`` listener callback.

This policy is considered during the creation of associations between data writers and data readers.
The value of both sides of the association must be compatible in order for an association to be created.
The deadline period of the data reader must be greater than or equal to the corresponding value of data writer.

The value of this policy may change after the associated entity is enabled.
In the case where the policy of a data reader or data writer is made, the change is successfully applied only if the change remains consistent with the remote end of all associations in which the reader or writer is participating.
If the policy of a topic is changed, it will affect only data readers and writers that are created after the change has been made.
Any existing readers or writers, and any existing associations between them, will not be affected by the topic policy value change.

.. _quality_of_service--lifespan:

LIFESPAN
========

..
    Sect<3.2.10>

The ``LIFESPAN`` QoS policy allows the application to specify when a sample expires.
Expired samples will not be delivered to subscribers.
This policy applies to the topic and data writer entities via the lifespan member of their respective QoS structures.
Below is the IDL related to the lifespan QoS policy.

.. code-block:: omg-idl

    struct LifespanQosPolicy {
      Duration_t duration;
    }

The default value of the ``duration`` member is infinite, which means samples never expire.
OpenDDS currently supports expired sample detection on the publisher side when using a ``DURABILITY`` ``kind`` other than ``VOLATILE``.
The current OpenDDS implementation may not remove samples from the data writer and data reader caches when they expire after being placed in the cache.

The value of this policy may be changed at any time.
Changes to this policy affect only data written after the change.

.. _quality_of_service--user-data:

USER_DATA
=========

..
    Sect<3.2.11>

The ``USER_DATA`` policy applies to the domain participant, data reader, and data writer entities via the user_data member of their respective QoS structures.
Below is the IDL related to the user data QoS policy:

.. code-block:: omg-idl

    struct UserDataQosPolicy {
      sequence<octet> value;
    };

By default, the ``value`` member is not set.
It can be set to any sequence of octets which can be used to attach information to the created entity.
The value of the ``USER_DATA`` policy is available in respective built-in topic data.
The remote application can obtain the information via the built-in topic and use it for its own purposes.
For example, the application could attach security credentials via the ``USER_DATA`` policy that can be used by the remote application to authenticate the source.

.. _quality_of_service--topic-data:

TOPIC_DATA
==========

..
    Sect<3.2.12>

The ``TOPIC_DATA`` policy applies to topic entities via the topic_data member of TopicQoS structures.
Below is the IDL related to the topic data QoS policy:

.. code-block:: omg-idl

    struct TopicDataQosPolicy {
      sequence<octet> value;
    };

By default, the ``value`` is not set.
It can be set to attach additional information to the created topic.
The value of the ``TOPIC_DATA`` policy is available in data writer, data reader, and topic built-in topic data.
The remote application can obtain the information via the built-in topic and use it in an application-defined way.

.. _quality_of_service--group-data:

GROUP_DATA
==========

..
    Sect<3.2.13>

The ``GROUP_DATA`` policy applies to the publisher and subscriber entities via the group_data member of their respective QoS structures.
Below is the IDL related to the group data QoS policy:

.. code-block:: omg-idl

    struct GroupDataQosPolicy {
      sequence<octet> value;
    };

By default, the ``value`` member is not set.
It can be set to attach additional information to the created entities.
The value of the ``GROUP_DATA`` policy is propagated via built-in topics.
The data writer built-in topic data contains the ``GROUP_DATA`` from the publisher and the data reader built-in topic data contains the ``GROUP_DATA`` from the subscriber.
The ``GROUP_DATA`` policy could be used to implement matching mechanisms similar to those of the ``PARTITION`` policy described in 1.1.6 except the decision could be made based on an application-defined policy.

.. _quality_of_service--transport-priority:

TRANSPORT_PRIORITY
==================

..
    Sect<3.2.14>

The ``TRANSPORT_PRIORITY`` policy applies to topic and data writer entities via the transport_priority member of their respective QoS policy structures.
Below is the IDL related to the TransportPriority QoS policy:

.. code-block:: omg-idl

    struct TransportPriorityQosPolicy {
      long value;
    };

The default value member of ``transport_priority`` is zero.
This policy is considered a hint to the transport layer to indicate at what priority to send messages.
Higher values indicate higher priority.
OpenDDS maps the priority value directly onto thread and DiffServ codepoint values.
A default priority of zero will not modify either threads or codepoints in messages.

OpenDDS will attempt to set the thread priority of the sending transport as well as any associated receiving transport.
Transport priority values are mapped from zero (default) through the maximum thread priority linearly without scaling.
If the lowest thread priority is different from zero, then it is mapped to the transport priority value of zero.
Where priority values on a system are inverted (higher numeric values are lower priority), OpenDDS maps these to an increasing priority value starting at zero.
Priority values lower than the minimum (lowest) thread priority on a system are mapped to that lowest priority.
Priority values greater than the maximum (highest) thread priority on a system are mapped to that highest priority.
On most systems, thread priorities can only be set when the process scheduler has been set to allow these operations.
Setting the process scheduler is generally a privileged operation and will require system privileges to perform.
On POSIX based systems, the system calls of ``sched_get_priority_min()`` and ``sched_get_priority_max()`` are used to determine the system range of thread priorities.

OpenDDS will attempt to set the DiffServ codepoint on the socket used to send data for the data writer if it is supported by the transport implementation.
If the network hardware honors the codepoint values, higher codepoint values will result in better (faster) transport for higher priority samples.
The default value of zero will be mapped to the (default) codepoint of zero.
Priority values from 1 through 63 are then mapped to the corresponding codepoint values, and higher priority values are mapped to the highest codepoint value (63).

OpenDDS does not currently support modifications of the transport_priority policy values after creation of the data writer.
This can be worked around by creating new data writers as different priority values are required.

.. _quality_of_service--latency-budget:

LATENCY_BUDGET
==============

..
    Sect<3.2.15>

The ``LATENCY_BUDGET`` policy applies to topic, data reader, and data writer entities via the latency_budget member of their respective QoS policy structures.
Below is the IDL related to the LatencyBudget QoS policy:

.. code-block:: omg-idl

    struct LatencyBudgetQosPolicy {
      Duration_t duration;
    };

The default value of ``duration`` is zero indicating that the delay should be minimized.
This policy is considered a hint to the transport layer to indicate the urgency of samples being sent.
OpenDDS uses the value to bound a delay interval for reporting unacceptable delay in transporting samples from publication to subscription.
This policy is used for monitoring purposes only at this time.
Use the ``TRANSPORT_PRIORITY`` policy to modify the sending of samples.
The data writer policy value is used only for compatibility comparisons and if left at the default value of zero will result in all requested duration values from data readers being matched.

An additional listener extension has been added to allow reporting delays in excess of the policy duration setting.
The ``OpenDDS::DCPS::DataReaderListener`` interface has an additional operation for notification that samples were received with a measured transport delay greater than the latency_budget policy duration.
The IDL for this method is:

.. code-block:: omg-idl

      struct BudgetExceededStatus {
        long total_count;
        long total_count_change;
        DDS::InstanceHandle_t last_instance_handle;
      };

      void on_budget_exceeded(
             in DDS::DataReader reader,
             in BudgetExceededStatus status);

To use the extended listener callback you will need to derive the listener implementation from the extended interface, as shown in the following code fragment:

.. code-block:: cpp

      class DataReaderListenerImpl
            : public virtual
              OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataReaderListener>

Then you must provide a non-null implementation for the ``on_budget_exceeded()`` operation.
Note that you will need to provide empty implementations for the following extended operations as well:

::

      on_subscription_disconnected()
      on_subscription_reconnected()
      on_subscription_lost()
      on_connection_deleted()

OpenDDS also makes the summary latency statistics available via an extended interface of the data reader.
This extended interface is located in the ``OpenDDS::DCPS`` module and the IDL is defined as:

.. code-block:: omg-idl

      struct LatencyStatistics {
        GUID_t        publication;
        unsigned long n;
        double        maximum;
        double        minimum;
        double        mean;
        double        variance;
      };

      typedef sequence<LatencyStatistics> LatencyStatisticsSeq;

      local interface DataReaderEx : DDS::DataReader {
        /// Obtain a sequence of statistics summaries.
        void get_latency_stats( inout LatencyStatisticsSeq stats);

        /// Clear any intermediate statistical values.
        void reset_latency_stats();

        /// Statistics gathering enable state.
        attribute boolean statistics_enabled;
      };

To gather this statistical summary data you will need to use the extended interface.
You can do so simply by dynamically casting the OpenDDS data reader pointer and calling the operations directly.
In the following example, we assume that reader is initialized correctly by calling ``DDS::Subscriber::create_datareader()``:

.. code-block:: cpp

      DDS::DataReader_var reader;
      // ...

      // To start collecting new data.
      dynamic_cast<OpenDDS::DCPS::DataReaderImpl*>(reader.in())->
        reset_latency_stats();
      dynamic_cast<OpenDDS::DCPS::DataReaderImpl*>(reader.in())->
        statistics_enabled(true);

      // ...

      // To collect data.
      OpenDDS::DCPS::LatencyStatisticsSeq stats;
      dynamic_cast<OpenDDS::DCPS::DataReaderImpl*>(reader.in())->
        get_latency_stats(stats);
      for (unsigned long i = 0; i < stats.length(); ++i)
      {
        std::cout << "stats[" << i << "]:" << std::endl;
        std::cout << "         n = " << stats[i].n << std::endl;
        std::cout << "       max = " << stats[i].maximum << std::endl;
        std::cout << "       min = " << stats[i].minimum << std::endl;
        std::cout << "      mean = " << stats[i].mean << std::endl;
        std::cout << "  variance = " << stats[i].variance << std::endl;
      }

.. _quality_of_service--entity-factory:

ENTITY_FACTORY
==============

..
    Sect<3.2.16>

The ``ENTITY_FACTORY`` policy controls whether entities are automatically enabled when they are created.
Below is the IDL related to the Entity Factory QoS policy:

.. code-block:: omg-idl

    struct EntityFactoryQosPolicy {
      boolean autoenable_created_entities;
    };

This policy can be applied to entities that serve as factories for other entities and controls whether or not entities created by those factories are automatically enabled upon creation.
This policy can be applied to the domain participant factory (as a factory for domain participants), domain participant (as a factory for publishers, subscribers, and topics), publisher (as a factory for data writers), or subscriber (as a factory for data readers).
The default value for the ``autoenable_created_entities`` member is ``true``, indicating that entities are automatically enabled when they are created.
Applications that wish to explicitly enable entities some time after they are created should set the value of the ``autoenable_created_entities`` member of this policy to ``false`` and apply the policy to the appropriate factory entities.
The application must then manually enable the entity by calling the entity’s ``enable()`` operation.

The value of this policy may be changed at any time.
Changes to this policy affect only entities created after the change.

.. _quality_of_service--presentation:

PRESENTATION
============

..
    Sect<3.2.17>

The ``PRESENTATION`` QoS policy controls how changes to instances by publishers are presented to data readers.
It affects the relative ordering of these changes and the scope of this ordering.
Additionally, this policy introduces the concept of coherent change sets.
Here is the IDL for the Presentation QoS:

.. code-block:: omg-idl

    enum PresentationQosPolicyAccessScopeKind {
      INSTANCE_PRESENTATION_QOS,
      TOPIC_PRESENTATION_QOS,
      GROUP_PRESENTATION_QOS
    };

    struct PresentationQosPolicy {
      PresentationQosPolicyAccessScopeKind access_scope;
      boolean coherent_access;
      boolean ordered_access;
    };

The scope of these changes (``access_scope``) specifies the level in which an application may be made aware:

* ``INSTANCE_PRESENTATION_QOS`` (the default) indicates that changes occur to instances independently.
  Instance access essentially acts as a no-op with respect to coherent_access and ordered_access.
  Setting either of these values to true has no observable affect within the subscribing application.

* ``TOPIC_PRESENTATION_QOS`` indicates that accepted changes are limited to all instances within the same data reader or data writer.

* ``GROUP_PRESENTATION_QOS`` indicates that accepted changes are limited to all instances within the same publisher or subscriber.

Coherent changes (``coherent_access``) allow one or more changes to an instance be made available to an associated data reader as a single change.
If a data reader does not receive the entire set of coherent changes made by a publisher, then none of the changes are made available.
The semantics of coherent changes are similar in nature to those found in transactions provided by many relational databases.
By default, ``coherent_access`` is ``false``.

Changes may also be made available to associated data readers in the order sent by the publisher (``ordered_access``).
This is similar in nature to the ``DESTINATION_ORDER QoS`` policy, however ``ordered_access`` permits data to be ordered independently of instance ordering.
By default, ``ordered_access`` is ``false``.

.. note:: This policy controls the ordering and scope of samples made available to the subscriber, but the subscriber application must use the proper logic in reading samples to guarantee the requested behavior.
  For more details, see Section 2.2.2.5.1.9 of the Version 1.4 DDS Specification.

.. _quality_of_service--destination-order:

DESTINATION_ORDER
=================

..
    Sect<3.2.18>

The ``DESTINATION_ORDER`` QoS policy controls the order in which samples within a given instance are made available to a data reader.
If a history depth of one (the default) is specified, the instance will reflect the most recent value written by all data writers to that instance.
Here is the IDL for the Destination Order Qos:

.. code-block:: omg-idl

    enum DestinationOrderQosPolicyKind {
      BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS,
      BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS
    };

    struct DestinationOrderQosPolicy {
      DestinationOrderQosPolicyKind kind;
    };

The ``BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS`` value (the default) indicates that samples within an instance are ordered in the order in which they were received by the data reader.
Note that samples are not necessarily received in the order sent by the same data writer.
To enforce this type of ordering, the ``BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS`` value should be used.

The ``BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS`` value indicates that samples within an instance are ordered based on a timestamp provided by the data writer.
It should be noted that if multiple data writers write to the same instance, care should be taken to ensure that clocks are synchronized to prevent incorrect ordering on the data reader.

.. _quality_of_service--writer-data-lifecycle:

WRITER_DATA_LIFECYCLE
=====================

..
    Sect<3.2.19>

The ``WRITER_DATA_LIFECYCLE`` QoS policy controls the lifecycle of data instances managed by a data writer.
Here is the IDL for the Writer Data Lifecycle QoS policy:

.. code-block:: omg-idl

    struct WriterDataLifecycleQosPolicy {
      boolean autodispose_unregistered_instances;
    };

When ``autodispose_unregistered_instances`` is set to ``true`` (the default), a data writer disposes an instance when it is unregistered.
In some cases, it may be desirable to prevent an instance from being disposed when an instance is unregistered.
This policy could, for example, allow an ``EXCLUSIVE`` data writer to gracefully defer to the next data writer without affecting the instance state.
Deleting a data writer implicitly unregisters all of its instances prior to deletion.

.. _quality_of_service--reader-data-lifecycle:

READER_DATA_LIFECYCLE
=====================

..
    Sect<3.2.20>

The ``READER_DATA_LIFECYCLE`` QoS policy controls the lifecycle of data instances managed by a data reader.
Here is the IDL for the Reader Data Lifecycle QoS policy:

.. code-block:: omg-idl

    struct ReaderDataLifecycleQosPolicy {
      Duration_t autopurge_nowriter_samples_delay;
      Duration_t autopurge_disposed_samples_delay;
    };

Normally, a data reader maintains data for all instances until there are no more associated data writers for the instance, the instance has been disposed, or the data has been taken by the user.

In some cases, it may be desirable to constrain the reclamation of these resources.
This policy could, for example, permit a late-joining data writer to prolong the lifetime of an instance in fail-over situations.

The ``autopurge_nowriter_samples_delay`` controls how long the data reader waits before reclaiming resources once an instance transitions to the ``NOT_ALIVE_NO_WRITERS`` state.
By default, ``autopurge_nowriter_samples_delay`` is infinite.

The ``autopurge_disposed_samples_delay`` controls how long the data reader waits before reclaiming resources once an instance transitions to the ``NOT_ALIVE_DISPOSED`` state.
By default, ``autopurge_disposed_samples_delay`` is infinite.

.. _quality_of_service--time-based-filter:

TIME_BASED_FILTER
=================

..
    Sect<3.2.21>

The ``TIME_BASED_FILTER`` QoS policy controls how often a data reader may be interested in changes in values to a data instance.
Here is the IDL for the Time Based Filter QoS:

.. code-block:: omg-idl

    struct TimeBasedFilterQosPolicy {
      Duration_t minimum_separation;
    };

An interval (``minimum_separation``) may be specified on the data reader.
This interval defines a minimum delay between instance value changes; this permits the data reader to throttle changes without affecting the state of the associated data writer.
By default, minimum_separation is zero, which indicates that no data is filtered.
This QoS policy does not conserve bandwidth as instance value changes are still sent to the subscriber process.
It only affects which samples are made available via the data reader.

.. _quality_of_service--ownership:

OWNERSHIP
=========

..
    Sect<3.2.22>

The ``OWNERSHIP`` policy controls whether more than one Data Writer is able to write samples for the same data-object instance.
Ownership can be ``EXCLUSIVE`` or ``SHARED``.
Below is the IDL related to the Ownership QoS policy:

.. code-block:: omg-idl

    enum OwnershipQosPolicyKind {
      SHARED_OWNERSHIP_QOS,
      EXCLUSIVE_OWNERSHIP_QOS
    };

    struct OwnershipQosPolicy {
      OwnershipQosPolicyKind kind;
    };

If the kind member is set to ``SHARED_OWNERSHIP_QOS``, more than one Data Writer is allowed to update the same data-object instance.
If the kind member is set to ``EXCLUSIVE_OWNERSHIP_QOS``, only one Data Writer is allowed to update a given data-object instance (i.e., the Data Writer is considered to be the *owner* of the instance) and associated Data Readers will only see samples written by that Data Writer.
The owner of the instance is determined by value of the ``OWNERSHIP_STRENGTH`` policy; the data writer with the highest value of strength is considered the owner of the data-object instance.
Other factors may also influence ownership, such as whether the data writer with the highest strength is “alive” (as defined by the ``LIVELINESS`` policy) and has not violated its offered publication deadline constraints (as defined by the ``DEADLINE`` policy).

.. _quality_of_service--ownership-strength:

OWNERSHIP_STRENGTH
==================

..
    Sect<3.2.23>

The ``OWNERSHIP_STRENGTH`` policy is used in conjunction with the ``OWNERSHIP`` policy, when the ``OWNERSHIP`` ``kind`` is set to ``EXCLUSIVE``.
Below is the IDL related to the Ownership Strength QoS policy:

.. code-block:: omg-idl

    struct OwnershipStrengthQosPolicy {
      long value;
    };

The value member is used to determine which Data Writer is the *owner* of the data-object instance.
The default value is zero.

.. _quality_of_service--policy-example:

**************
Policy Example
**************

..
    Sect<3.3>

The following sample code illustrates some policies being set and applied for a publisher.

.. code-block:: cpp

          DDS::DataWriterQos dw_qos;
          pub->get_default_datawriter_qos (dw_qos);

          dw_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;

          dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
          dw_qos.reliability.max_blocking_time.sec = 10;
          dw_qos.reliability.max_blocking_time.nanosec = 0;

          dw_qos.resource_limits.max_samples_per_instance = 100;

          DDS::DataWriter_var dw =
            pub->create_datawriter(topic,
                                   dw_qos,
                                   0,   // No listener
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

This code creates a publisher with the following qualities:

* ``HISTORY`` set to Keep All

* ``RELIABILITY`` set to Reliable with a maximum blocking time of 10 seconds

* The maximum samples per instance resource limit set to 100

This means that when 100 samples are waiting to be delivered, the writer can block up to 10 seconds before returning an error code.
These same QoS settings on the Data Reader side would mean that up to 100 unread samples are queued by the framework before any are rejected.
Rejected samples are dropped and the SampleRejectedStatus is updated.

.. rubric:: Footnotes

.. [#footnote1]

   For OpenDDS versions, up to 2.0, the default reliability kind for data writers is best effort.
   For versions 2.0.1 and later, this is changed to reliable (to conform to the DDS specification).
