.. _quality_of_service:
.. _qos:

##################
Quality of Service
##################

..
    Sect<3>
    Sect<3.1>
    Sect<3.2>

Quality of Service (QoS) policies are sets of requested policies for how :term:`entities <Entity>` should behave.
Each policy defines a structure to specify its data.
Each entity supports a subset of the policies and defines a QoS structure that is composed of the supported policy structures.
The set of allowable policies for a given entity is constrained by the policy structures nested in its QoS structure.
For example, the Publisher's QoS structure is defined in the specification's IDL as follows:

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
See :ref:`qos-defaults` for examples of how to obtain the default QoS policies for various entity types.

.. _qos-changing:

*********************
Changing QoS Policies
*********************

Applications can change the QoS of any entity by calling the ``set_qos()`` operation on the entity.
If the QoS policy values are either invalid or have conflicting polices, then ``set_qos()`` returns ``DDS::RETCODE_INCONSISTENT_POLICY``.
If the application attempts to change a QoS policy that is immutable (not changeable), then ``set_qos()`` returns ``DDS::RETCODE_IMMUTABLE_POLICY``.

If the ``set_qos`` is successful, existing associations are removed if they are no longer compatible and new associations are added if they become compatible.
The policies specify if they are mutable or not and how they affect associations.
The application can detect failed associations caused by incompatible QoS using :ref:`conditions_and_listeners--offered-incompatible-qos-status` for writers and :ref:`conditions_and_listeners--requested-incompatible-qos-status` for readers.

.. _qos-defaults:
.. _quality_of_service--default-qos-policy-values:

*************************
Default QoS Policy Values
*************************

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

********
Policies
********

.. _qos-liveliness:
.. _quality_of_service--liveliness:

Liveliness QoS
==============

..
    Sect<3.2.2>

The liveliness QoS policy controls when and how the service determines whether participants are considered "alive", meaning they are still reachable and active.
This policy applies to the topic, data reader, and data writer entities via the ``liveliness`` member of their respective QoS structures.
Setting this policy on a topic means it is in effect for all data readers and data writers on that topic.

.. important::

  This policy is :ref:`immutable <qos-changing>` and affects :ref:`association <qos-liveliness-association>`.

  IDL:

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

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`Topic`, :term:`DataWriter`, and :term:`DataReader`

       - ``kind``

         ``lease_duration.sec``

         ``lease_duration.nanosec``

       - ``AUTOMATIC_LIVELINESS_QOS``

         ``DURATION_INFINITE_SEC``

         ``DURATION_INFINITE_NSEC``

  Specification: :omgspec:`dds:2.2.3.11 LIVELINESS`

The ``kind`` member setting indicates whether liveliness is asserted automatically by the service or manually by the specified entity.
A setting of ``AUTOMATIC_LIVELINESS_QOS`` means that the service will send a liveliness indication if the participant has not sent any network traffic for the ``lease_duration``.
The ``MANUAL_BY_PARTICIPANT_LIVELINESS_QOS`` or ``MANUAL_BY_TOPIC_LIVELINESS_QOS`` setting means the specified entity (data writer for the "by topic" setting or domain participant for the "by participant" setting) must either write a :term:`sample` or manually assert its liveliness within a specified heartbeat interval.
The desired heartbeat interval is specified by the ``lease_duration`` member.
The default lease duration is a pre-defined infinite value, which disables any liveliness testing.

To manually assert liveliness without publishing a sample, the application must call the ``assert_liveliness()`` operation on the data writer (for the "by topic" setting) or on the domain participant (for the "by participant" setting) within the specified heartbeat interval.

Data writers specify (*offer*) their own liveliness criteria and data readers specify (*request*) the desired liveliness of their writers.
Writers that are not heard from within the lease duration (either by writing a sample or by asserting liveliness) cause a change in the :ref:`LIVELINESS_CHANGED_STATUS <conditions_and_listeners--liveliness-changed-status>` communication status and notification to the application (e.g., by calling the data reader listener's ``on_liveliness_changed()`` callback operation or by signaling any related wait sets).

.. _qos-liveliness-association:

This policy is considered during the establishment of associations between data writers and data readers.
The value of both sides of the association must be compatible in order for an association to be established.
Compatibility is determined by comparing the data reader's requested liveliness with the data writer's offered liveliness.
Both the kind of liveliness (automatic, manual by topic, manual by participant) and the value of the lease duration are considered in determining compatibility.
The writer's offered kind of liveliness must be greater than or equal to the reader's requested kind of liveliness.
The liveliness kind values are ordered as follows:

::

    MANUAL_BY_TOPIC_LIVELINESS_QOS >
    MANUAL_BY_PARTICIPANT_LIVELINESS_QOS >
    AUTOMATIC_LIVELINESS_QOS

In addition, the writer's offered lease duration must be less than or equal to the reader's requested lease duration.
Both of these conditions must be met for the offered and requested liveliness policy settings to be considered compatible and the association established.

.. _qos-reliability:
.. _quality_of_service--reliability:

Reliability QoS
===============

..
    Sect<3.2.3>

The reliability QoS policy applies to the topic, data reader, and data writer entities via the ``reliability`` member of their respective QoS structures.
This policy controls how data readers and writers treat the :term:`sample`\s they process.

.. important::

  This policy is :ref:`immutable <qos-changing>` and affects :ref:`association <qos-reliability-association>`.

  IDL:

  .. code-block:: omg-idl

    enum ReliabilityQosPolicyKind {
      BEST_EFFORT_RELIABILITY_QOS,
      RELIABLE_RELIABILITY_QOS
    };

    struct ReliabilityQosPolicy {
      ReliabilityQosPolicyKind kind;
      Duration_t max_blocking_time;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`Topic` and :term:`DataReader`

       - ``kind``

         ``max_blocking_time.sec``

         ``max_blocking_time.nanosec``

       - ``BEST_EFFORT_RELIABILITY_QOS``

         ``DURATION_INFINITE_SEC``

         ``DURATION_INFINITE_NSEC``

     * - :term:`DataWriter`

       - ``kind``

         ``max_blocking_time.sec``

         ``max_blocking_time.nanosec``

       - ``RELIABLE_RELIABILITY_QOS`` [#footnote1]_

         ``0``

         ``100000000`` (100 ms)

  Specification: :omgspec:`dds:2.2.3.14 RELIABILITY`

The "best effort" value (``BEST_EFFORT_RELIABILITY_QOS``) makes no promises as to the reliability of the samples and could be expected to drop samples under some circumstances.
The "reliable" value (``RELIABLE_RELIABILITY_QOS``) indicates that the service should eventually deliver all values to eligible data readers.

The ``max_blocking_time`` member of this policy is used when the :ref:`qos-history` policy is set to "keep all" and the writer is unable to proceed because of :ref:`qos-resource-limits`.
When this situation occurs and the writer blocks for more than the specified time, then the write fails with a ``DDS::RETCODE_TIMEOUT`` return code.
The default for this policy for data readers and topics is "best effort", while the default value for data writers is "reliable".

.. _qos-reliability-association:

This policy is considered during the creation of associations between data writers and data readers.
The value of both sides of the association must be compatible in order for an association to be created.
The reliability kind of data writer must be greater than or equal to the value of data reader.
In other words, all combinations are valid except that a reliable reader can only associate with a reliable writer, not a "best effort" one.

.. _qos-history:
.. _quality_of_service--history:

History QoS
===========

..
    Sect<3.2.4>

The history QoS policy determines how :term:`sample`\s are held in the data writer and data reader for a particular :term:`instance`.
For data writers these samples are held until the publisher retrieves them and successfully sends them to all connected subscribers.
For data readers these samples are held until :ref:`taken <getting_started--reading-multiple-samples>` by the application.
This policy applies to the topic, data reader, and data writer entities via the ``history`` member of their respective QoS structures.

.. important::

  This policy is :ref:`immutable <qos-changing>` and does not affect association.

  IDL:

  .. code-block:: omg-idl

    enum HistoryQosPolicyKind {
      KEEP_LAST_HISTORY_QOS,
      KEEP_ALL_HISTORY_QOS
    };

    struct HistoryQosPolicy {
      HistoryQosPolicyKind kind;
      long depth;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`Topic`, :term:`DataWriter`, and :term:`DataReader`

       - ``kind``

         ``depth``

       - ``KEEP_LAST_HISTORY_QOS``

         ``1``

  Specification: :omgspec:`dds:2.2.3.18 HISTORY`

The "keep all" value (``KEEP_ALL_HISTORY_QOS``) specifies that all possible samples for that instance should be kept.
When "keep all" is specified and the number of unread samples is equal to the :ref:`qos-resource-limits` field of ``max_samples_per_instance`` then any incoming samples are rejected.

The "keep last" value (``KEEP_LAST_HISTORY_QOS``) specifies that only the last ``depth`` values should be kept.
When a data writer contains depth samples of a given instance, a write of new samples for that instance are queued for delivery and the oldest unsent samples are discarded.
When a data reader contains depth samples of a given instance, any incoming samples for that instance are kept and the oldest samples are discarded.

.. _qos-durability:
.. _quality_of_service--durability:

Durability QoS
==============

..
    Sect<3.2.5>

The durability QoS policy controls whether data writers should maintain :term:`sample`\s after they have been sent to known subscribers.
This policy applies to the topic, data reader, and data writer entities via the ``durability`` member of their respective QoS structures.

.. important::

  This policy is :ref:`immutable <qos-changing>` and affects :ref:`association <qos-durability-association>`.

  IDL:

  .. code-block:: omg-idl

    enum DurabilityQosPolicyKind {
      VOLATILE_DURABILITY_QOS,         // Least Durability
      TRANSIENT_LOCAL_DURABILITY_QOS,
      TRANSIENT_DURABILITY_QOS,
      PERSISTENT_DURABILITY_QOS        // Greatest Durability
    };

    struct DurabilityQosPolicy {
      DurabilityQosPolicyKind kind;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`Topic`, :term:`DataWriter`, and :term:`DataReader`

       - ``kind``

       - ``VOLATILE_DURABILITY_QOS``

  Specification: :omgspec:`dds:2.2.3.4 DURABILITY`

.. _VOLATILE_DURABILITY_QOS:

A durability kind of ``VOLATILE_DURABILITY_QOS`` means samples are discarded after being sent to all known subscribers.
As a side effect, subscribers cannot recover samples sent before they connect.

A durability kind of ``TRANSIENT_LOCAL_DURABILITY_QOS`` means that data readers that are associated/connected with a data writer will be sent all of the samples in the data writer's history.

A durability kind of ``TRANSIENT_DURABILITY_QOS`` means that samples outlive a data writer and last as long as the process is alive.
The samples are kept in memory, but are not persisted to permanent storage.
A data reader subscribed to the same topic and partition within the same domain will be sent all of the cached samples that belong to the same topic/partition.

.. _PERSISTENT_DURABILITY_QOS:

A durability kind of ``PERSISTENT_DURABILITY_QOS`` provides basically the same functionality as transient durability except the cached samples are persisted and will survive process destruction.

When transient or persistent durability is specified, the :ref:`qos-durability-service` QoS policy specifies additional tuning parameters for the durability cache.

.. _qos-durability-association:

The durability policy is considered during the creation of associations between data writers and data readers.
The value of both sides of the association must be compatible in order for an association to be created.
The durability kind value of the data writer must be greater than or equal to the corresponding value of the data reader.
The durability kind values are ordered as follows:

::

    PERSISTENT_DURABILITY_QOS >
    TRANSIENT_DURABILITY_QOS >
    TRANSIENT_LOCAL_DURABILITY_QOS >
    VOLATILE_DURABILITY_QOS

.. _qos-durability-service:
.. _quality_of_service--durability-service:

Durability Service QoS
======================

..
    Sect<3.2.6>

The durability service QoS policy controls deletion of :term:`sample`\s in the transient or persistent durability caches.
This policy applies to the topic and data writer entities via the ``durability_service`` member of their respective QoS structures and provides a way to specify :ref:`qos-history` and :ref:`qos-resource-limits` for the sample cache.

.. important::

  This policy is :ref:`immutable <qos-changing>` and does not affect association.

  IDL:

  .. code-block:: omg-idl

    struct DurabilityServiceQosPolicy {
      Duration_t              service_cleanup_delay;
      HistoryQosPolicyKind    history_kind;
      long                    history_depth;
      long                    max_samples;
      long                    max_instances;
      long                    max_samples_per_instance;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`Topic` and :term:`DataWriter`

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

  Specification: :omgspec:`dds:2.2.3.5 DURABILITY_SERVICE`

The members are analogous to, although independent of, those found in the :ref:`qos-history` and :ref:`qos-resource-limits` policies.
The ``service_cleanup_delay`` can be set to a desired value.
By default, it is set to zero, which means never clean up cached samples.

.. _qos-resource-limits:
.. _quality_of_service--resource-limits:

Resource Limits QoS
===================

..
    Sect<3.2.7>

The resource limits determines the amount of resources the service can consume in order to meet the requested QoS.
This policy applies to the topic, data reader, and data writer entities via the ``resource_limits`` member of their respective QoS structures.

.. important::

  This policy is :ref:`immutable <qos-changing>` and does not affect association.

  IDL:

  .. code-block:: omg-idl

    struct ResourceLimitsQosPolicy {
      long max_samples;
      long max_instances;
      long max_samples_per_instance;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`Topic`, :term:`DataWriter`, and :term:`DataReader`

       - ``max_samples``

         ``max_instances``

         ``max_samples_per_instance``

       - ``LENGTH_UNLIMITED``

         ``LENGTH_UNLIMITED``

         ``LENGTH_UNLIMITED``

  Specification: :omgspec:`dds:2.2.3.19 RESOURCE_LIMITS`

The ``max_samples`` member specifies the maximum number of samples a single data writer or data reader can manage across all of its instances.
The ``max_instances`` member specifies the maximum number of instances that a data writer or data reader can manage.
The ``max_samples_per_instance`` member specifies the maximum number of samples that can be managed for an individual instance in a single data writer or data reader.

Resources are used by the data writer to queue samples written to the data writer but not yet sent to all data readers because of backpressure from the transport.
Resources are used by the data reader to queue samples that have been received, but not yet read/taken from the data reader.

.. _qos-partition:
.. _quality_of_service--partition:

Partition QoS
=============

..
    Sect<3.2.8>

The partition QoS policy allows the creation of logical partitions within a :term:`domain`.
Data readers and data writers can only be associated if they have at least one matched partition string.
This policy applies to the publisher and subscriber entities via the ``partition`` member of their respective QoS structures.

.. important::

  This policy is :ref:`mutable <qos-changing>` and :ref:`affects association <qos-partition-association>`.

  IDL:

  .. code-block:: omg-idl

    struct PartitionQosPolicy {
      StringSeq name;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`Publisher` and :term:`Subscriber`

       - ``name``

       - (empty sequence)

  Specification: :omgspec:`dds:2.2.3.13 PARTITION`

The default partition name is an empty string and causes the entity to participate in the default partition.
The partition names may contain wildcard characters as they are :ref:`fnmatch-exprs`.

.. _qos-partition-association:

The establishment of data reader and data writer associations depends on matching partition strings on the publication and subscription ends.
Failure to match partitions is not considered a failure and does not trigger any callbacks or set any status values.

.. _qos-deadline:
.. _quality_of_service--deadline:

Deadline QoS
============

..
    Sect<3.2.9>

The deadline QoS policy allows the application to detect when :term:`sample`\s are not written or read within a specified amount of time.
This policy applies to the topic, data writer, and data reader entities via the ``deadline`` member of their respective QoS structures.

.. important::

  This policy is :ref:`mutable <qos-changing>` and :ref:`affects association <qos-deadline-association>`.

  IDL:

  .. code-block:: omg-idl

    struct DeadlineQosPolicy {
      Duration_t period;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`Topic`, :term:`DataWriter`, and :term:`DataReader`

       - ``period.sec``

         ``period.nanosec``

       - ``DURATION_INFINITE_SEC``

         ``DURATION_INFINITE_NSEC``

  Specification: :omgspec:`dds:2.2.3.7 DEADLINE`

The default value of the ``period`` member is infinite, which requires no behavior.
When this policy is set to a finite value, then the data writer monitors the changes to data made by the application and indicates failure to honor the policy by setting the corresponding status condition and triggering the ``on_offered_deadline_missed()`` listener callback.
A data reader that detects that the data has not changed before the period has expired sets the corresponding status condition and triggers the ``on_requested_deadline_missed()`` listener callback.

.. _qos-deadline-association:

This policy is considered during the creation of associations between data writers and data readers.
The value of both sides of the association must be compatible in order for an association to be created.
The deadline period of the data reader must be greater than or equal to the corresponding value of data writer.

The value of this policy may change after the associated entity is enabled.
In the case where the policy of a data reader or data writer is made, the change is successfully applied only if the change remains consistent with the remote end of all associations in which the reader or writer is participating.
If the policy of a topic is changed, it will only affect data readers and writers that are created after the change has been made.
Any existing readers or writers, and any existing associations between them, will not be affected by the topic policy value change.

.. _qos-lifespan:
.. _quality_of_service--lifespan:

Lifespan QoS
============

..
    Sect<3.2.10>

The lifespan QoS policy allows the application to specify when a :term:`sample` expires.
Expired samples will not be delivered to subscribers.
This policy applies to the topic and data writer entities via the ``lifespan`` member of their respective QoS structures.

.. important::

  This policy is :ref:`mutable <qos-changing>` and does not affect association.

  IDL:

  .. code-block:: omg-idl

    struct LifespanQosPolicy {
      Duration_t duration;
    }

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`Topic` and :term:`DataWriter`

       - ``duration.sec``

         ``duration.nanosec``

       - ``DURATION_INFINITE_SEC``

         ``DURATION_INFINITE_NSEC``

  Specification: :omgspec:`dds:2.2.3.16 LIFESPAN`

The default value of the ``duration`` member is infinite, which means samples never expire.

.. note::

  OpenDDS currently supports expired sample detection on the publisher side when using a :ref:`qos-durability` ``kind`` other than :ref:`VOLATILE_DURABILITY_QOS <VOLATILE_DURABILITY_QOS>`.
  The current OpenDDS implementation may not remove samples from the data writer and data reader caches when they expire after being placed in the cache.

The value of this policy may be changed at any time, but only affects data written after the change.

.. _qos-user-data:
.. _quality_of_service--user-data:

User Data QoS
=============

..
    Sect<3.2.11>

The user data QoS policy can be used to attach arbitrary information to the created :term:`entity`.
It is not available to the user when using :ref:`static-disc`.
This policy applies to the domain participant, data reader, and data writer entities via the ``user_data`` member of their respective QoS structures.

.. important::

  This policy is :ref:`mutable <qos-changing>` and does not affect association.

  IDL:

  .. code-block:: omg-idl

    struct UserDataQosPolicy {
      sequence<octet> value;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`DomainParticipant`, :term:`DataWriter`, :term:`DataReader`

       - ``value``

       - (empty sequence)

  Specification: :omgspec:`dds:2.2.3.1 USER_DATA`

The value of the policy is available in respective :ref:`built-in topic data <bit>`.
The value's use is defined by the application.
For example, the application could attach security credentials via the policy that can be used by the remote application to authenticate the source. [#user_data_ex]_

.. warning::

  When using :ref:`dds_security`, the user data of a participant can be leaked in unsecured discovery messages.
  Enabling :cfg:prop:`[rtps_discovery]SecureParticipantUserData` will only send and provide the real user data when it can be securely sent.
  This is an OpenDDS-specific extension.

.. _qos-topic-data:
.. _quality_of_service--topic-data:

Topic Data QoS
==============

..
    Sect<3.2.12>

The topic data QoS policy can be used to attach arbitrary information to the created :term:`topic`.
This policy applies to topic entities via the ``topic_data`` member of ``TopicQoS`` structures.

.. important::

  This policy is :ref:`mutable <qos-changing>` and does not affect association.

  IDL:

  .. code-block:: omg-idl

    struct TopicDataQosPolicy {
      sequence<octet> value;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`Topic`

       - ``value``

       - (empty sequence)


  Specification: :omgspec:`dds:2.2.3.2 TOPIC_DATA`

The value of the topic data policy is available in data writer, data reader, and topic :ref:`built-in topic data <bit>`.
The value's use is defined by the application.

.. _qos-group-data:
.. _quality_of_service--group-data:

Group Data QoS
==============

..
    Sect<3.2.13>

The group data QoS policy can be used to attach arbitrary information to the created :term:`entity`.
This policy applies to the publisher and subscriber entities via the ``group_data`` member of their respective QoS structures.

.. important::

  This policy is :ref:`mutable <qos-changing>` and does not affect association.

  IDL:

  .. code-block:: omg-idl

    struct GroupDataQosPolicy {
      sequence<octet> value;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`Publisher` and :term:`Subscriber`

       - ``value``

       - (empty sequence)

  Specification: :omgspec:`dds:2.2.3.3 GROUP_DATA`

The value of the group data policy is propagated via :ref:`built-in topics <bit>` using the writer built-in topic data for the publisher and the reader built-in topic data for the subscriber.
The value's use is defined by the application.
This could be used to implement matching mechanisms similar to those of the :ref:`qos-partition` except the decision could be made based on an application-defined policy.

.. _qos-transport-priority:
.. _quality_of_service--transport-priority:

Transport Priority QoS
======================

..
    Sect<3.2.14>

The transport priority QoS policy is considered a hint to the transport layer to indicate at what priority to send messages.
This policy applies to topic and data writer entities via the ``transport_priority`` member of their respective QoS policy structures.

.. important::

  OpenDDS currently only implements this for the :ref:`tcp <tcp-transport>` and :ref:`udp <udp-transport>` transports.

  This policy is :ref:`immutable <qos-changing>` and does not affect association.
  This is opposed to the DDS specification, which specifies that it's mutable.
  OpenDDS does not currently support modifications of the transport priority policy values after creation of the data writer.
  This can be worked around by creating new data writers as different priority values are required.

  IDL:

  .. code-block:: omg-idl

    struct TransportPriorityQosPolicy {
      long value;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`Topic` and :term:`DataWriter`

       - ``value``

       - ``0``

  Specification: :omgspec:`dds:2.2.3.15 TRANSPORT_PRIORITY`

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
On POSIX based systems, the system calls of :manpage:`sched_get_priority_min(2)` and :manpage:`sched_get_priority_max(2)` are used to determine the system range of thread priorities.

OpenDDS will attempt to set the DiffServ codepoint on the socket used to send data for the data writer if it is supported by the transport implementation.
If the network hardware honors the codepoint values, higher codepoint values will result in better (faster) transport for higher priority samples.
The default value of zero will be mapped to the (default) codepoint of zero.
Priority values from 1 through 63 are then mapped to the corresponding codepoint values, and higher priority values are mapped to the highest codepoint value (63).

.. _qos-latency-budget:
.. _quality_of_service--latency-budget:

Latency Budget QoS
==================

..
    Sect<3.2.15>

The latency budget QoS policy is considered a hint to the transport layer to indicate the urgency of :term:`sample`\s being sent.
This policy applies to topic, data reader, and data writer entities via the ``latency_budget`` member of their respective QoS policy structures.

.. important::

  This policy is :ref:`mutable <qos-changing>` and :ref:`affects association <qos-latency-budget-association>`.

  IDL:

  .. code-block:: omg-idl

    struct LatencyBudgetQosPolicy {
      Duration_t duration;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`Topic`, :term:`DataWriter`, and :term:`DataReader`

       - ``duration.sec``

         ``duration.nanosec``

       - ``DURATION_ZERO_SEC``

         ``DURATION_ZERO_NSEC``

  Specification: :omgspec:`dds:2.2.3.8 LATENCY_BUDGET`

The default value of ``duration`` is zero indicating that the delay should be minimized.
OpenDDS uses the value to bound a delay interval for reporting unacceptable delay in transporting samples from publication to subscription.
This policy is used for monitoring purposes only at this time.
Use :ref:`qos-transport-priority` to modify the sending priority of samples.

There is an OpenDDS-specific :ref:`conditions_and_listeners--budget-exceeded-status` that reports when the duration is exceeded.

.. _qos-latency-budget-association:

The data writer policy value is used only for compatibility comparisons and if left at the default value of zero will result in all requested duration values from data readers being matched.
The data writer policy value must be greater or equal to a data reader policy value for an association to occur or continue to exist.

.. _qos-entity-factory:
.. _quality_of_service--entity-factory:

Entity Factory QoS
==================

..
    Sect<3.2.16>

The entity factory QoS policy controls whether :term:`entities <entity>` are automatically enabled by their factories when they are created.
This policy applies to domain participant factory (as a factory for domain participants), domain participant (as a factory for publishers, subscribers, and topics), publisher (as a factory for data writers), or subscriber (as a factory for data readers).

.. important::

  This policy is :ref:`mutable <qos-changing>` and does not affect association.

  IDL:

  .. code-block:: omg-idl

    struct EntityFactoryQosPolicy {
      boolean autoenable_created_entities;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - ``DomainParticipantFactory``, :term:`DomainParticipant`, :term:`Publisher`, and :term:`Subscriber`

       - ``autoenable_created_entities``

       - ``true``

  Specification: :omgspec:`dds:2.2.3.20 ENTITY_FACTORY`

Applications that wish to explicitly enable entities some time after they are created should set the value of the ``autoenable_created_entities`` member of this policy to ``false`` and apply the policy to the appropriate factory entities.
The application must then manually enable the entity by calling the entity's ``enable()`` operation.
One use of setting this policy to ``false`` is :ref:`binding specfic transport configurations to readers and writers <run_time_configuration--using-multiple-configurations>`.

The value of this policy may be changed at any time, but this only affects entities created after the change.

.. _qos-presentation:
.. _quality_of_service--presentation:

Presentation QoS
================

..
    Sect<3.2.17>

The presentation QoS policy controls how changes to :term:`instance`\s by publishers are presented to data readers.
It affects the relative ordering of these changes and the scope of this ordering.

.. important::

  This policy is :ref:`immutable <qos-changing>` and affects :ref:`association <qos-presentation-association>`.

  IDL:

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

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`Publisher` and :term:`Subscriber`

       - ``access_scope``

         ``coherent_access``

         ``ordered_access``

       - ``INSTANCE_PRESENTATION_QOS``

         ``0``

         ``0``

  Specification: :omgspec:`dds:2.2.3.6 PRESENTATION`

The scope of these changes (``access_scope``) specifies the level in which an application may be made aware:

* ``INSTANCE_PRESENTATION_QOS`` (the default) indicates that changes occur to instances independently.
  Instance access essentially acts as a no-op with respect to ``coherent_access`` and ``ordered_access``.
  Setting either of these values to true has no observable affect within the subscribing application.

* ``TOPIC_PRESENTATION_QOS`` indicates that accepted changes are limited to all instances within the same data reader or data writer.

* ``GROUP_PRESENTATION_QOS`` indicates that accepted changes are limited to all instances within the same publisher or subscriber.

Coherent changes (``coherent_access``) allow one or more changes to an instance be made available to an associated data reader as a single change.
If a data reader does not receive the entire set of coherent changes made by a publisher, then none of the changes are made available.
The semantics of coherent changes are similar in nature to those found in transactions provided by many relational databases.
By default, ``coherent_access`` is ``false``.

Changes may also be made available to associated data readers in the order sent by the publisher (``ordered_access``).
This is similar in nature to the :ref:`qos-destination-order` policy, however ``ordered_access`` permits data to be ordered independently of instance ordering.
By default, ``ordered_access`` is ``false``.

.. note:: This policy controls the ordering and scope of :term:`sample`\s made available to the subscriber, but the subscriber application must use the proper logic in reading samples to guarantee the requested behavior.
  For more details, see :omgspec:`dds:2.2.2.5.1.9 Data access patterns`.

.. _qos-presentation-association:

For a reader and writer to associate, all the following must be true:

- The writer's ``access_scope`` must be greater or equal to the reader's.
- The writer's ``coherent_access`` must be ``false`` or else both the reader's and writer's ``coherent_access`` must both be ``true``.
- The writer's ``ordered_access`` must be ``false`` or else both the reader's and writer's ``ordered_access`` must both be ``true``.

.. _qos-destination-order:
.. _quality_of_service--destination-order:

Destination Order QoS
=====================

..
    Sect<3.2.18>

The destination order QoS policy controls the order in which :term:`sample`\s within a given :term:`instance` are made available to a data reader.

.. important::

  This policy is :ref:`immutable <qos-changing>` and affects :ref:`association <qos-destination-order-association>`.

  IDL:

  .. code-block:: omg-idl

    enum DestinationOrderQosPolicyKind {
      BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS,
      BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS
    };

    struct DestinationOrderQosPolicy {
      DestinationOrderQosPolicyKind kind;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`Topic`, :term:`DataWriter`, and :term:`DataReader`

       - ``kind``

       - ``BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS``

  Specification: :omgspec:`dds:2.2.3.17 DESTINATION_ORDER`

If a :ref:`qos-history` ``depth`` of one (the default) is specified, the instance will reflect the most recent value written by all data writers to that instance.

The ``BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS`` value (the default) indicates that samples within an instance are ordered in the order in which they were received by the data reader.
Note that samples are not necessarily received in the order sent by the same data writer.
To enforce this type of ordering, the ``BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS`` value should be used.

The ``BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS`` value indicates that samples within an instance are ordered based on a timestamp provided by the data writer.
It should be noted that if multiple data writers write to the same instance, care should be taken to ensure that clocks are synchronized to prevent incorrect ordering on the data reader.

.. _qos-destination-order-association:

For a reader and writer to associate, the writer's ``kind`` must be greater or equal to the reader's.
In other words, if the reader is ``BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS``, then it the writer can be either value.
If the reader is ``BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS``, then the writer must also be ``BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS``.

.. _qos-writer-data-lifecycle:
.. _quality_of_service--writer-data-lifecycle:

Writer Data Lifecycle QoS
=========================

..
    Sect<3.2.19>

The writer data lifecycle QoS policy controls the lifecycle of :term:`instance`\s managed by a :term:`DataWriter`.

.. important::

  This policy is :ref:`mutable <qos-changing>` and does not affect association.

  IDL:

  .. code-block:: omg-idl

    struct WriterDataLifecycleQosPolicy {
      boolean autodispose_unregistered_instances;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`DataWriter`

       - ``autodispose_unregistered_instances``

       - ``true``

  Specification: :omgspec:`dds:2.2.3.21 WRITER_DATA_LIFECYCLE`

When ``autodispose_unregistered_instances`` is set to ``true`` (the default), a data writer :term:`dispose`\s an instance when it is :term:`unregister`\ed.
In some cases, it may be desirable to prevent an instance from being disposed when an instance is unregistered.
This policy could, for example, allow an :ref:`exclusive ownership <qos-ownership>` data writer to gracefully defer to the next data writer without affecting the instance state.
Deleting a data writer implicitly unregisters all of its instances prior to deletion.

.. _qos-reader-data-lifecycle:
.. _quality_of_service--reader-data-lifecycle:

Reader Data Lifecycle QoS
=========================

..
    Sect<3.2.20>

The reader data lifecycle QoS policy controls the lifecycle of :term:`instance`\s managed by a :term:`DataReader`.

.. important::

  This policy is :ref:`mutable <qos-changing>` and does not affect association.

  IDL:

  .. code-block:: omg-idl

    struct ReaderDataLifecycleQosPolicy {
      Duration_t autopurge_nowriter_samples_delay;
      Duration_t autopurge_disposed_samples_delay;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`DataReader`

       - ``autopurge_nowriter_samples_delay.sec``

         ``autopurge_nowriter_samples_delay.nanosec``

         ``autopurge_disposed_samples_delay.sec``

         ``autopurge_disposed_samples_delay.nanosec``

       - ``DURATION_INFINITE_SEC``

         ``DURATION_INFINITE_NSEC``

         ``DURATION_INFINITE_SEC``

         ``DURATION_INFINITE_NSEC``

  Specification: :omgspec:`dds:2.2.3.22 READER_DATA_LIFECYCLE`

Normally, a data reader maintains data for all instances until there are no more associated data writers for the instance, the instance has been :term:`dispose`\d, or the data has been :ref:`taken <getting_started--reading-multiple-samples>` by the user.

In some cases, it may be desirable to constrain the reclamation of these resources.
This policy could, for example, permit a late-joining data writer to prolong the lifetime of an instance in fail-over situations.

The ``autopurge_nowriter_samples_delay`` controls how long the data reader waits before reclaiming resources once an instance transitions to the ``NOT_ALIVE_NO_WRITERS`` state.
By default, ``autopurge_nowriter_samples_delay`` is infinite.

The ``autopurge_disposed_samples_delay`` controls how long the data reader waits before reclaiming resources once an instance transitions to the ``NOT_ALIVE_DISPOSED`` state.
By default, ``autopurge_disposed_samples_delay`` is infinite.

.. _qos-time-based-filter:
.. _quality_of_service--time-based-filter:

Time Based Filter QoS
=====================

..
    Sect<3.2.21>

The time based filter QoS policy controls how often a :term:`DataReader` may be interested in changes in values to an :term:`instance`.

.. important::

  This policy is :ref:`mutable <qos-changing>` and does not affect association.

  IDL:

  .. code-block:: omg-idl

    struct TimeBasedFilterQosPolicy {
      Duration_t minimum_separation;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`DataReader`

       - ``minimum_separation.sec``

         ``minimum_separation.nanosec``

       - ``DURATION_ZERO_SEC``

         ``DURATION_ZERO_NSEC``

  Specification: :omgspec:`dds:2.2.3.12 TIME_BASED_FILTER`

An interval (``minimum_separation``) may be specified on the data reader.
This interval defines a minimum delay between instance value changes; this permits the data reader to throttle changes without affecting the state of the associated data writer.
By default, ``minimum_separation`` is zero, which indicates that no data is filtered.
This QoS policy does not conserve bandwidth as instance value changes are still sent to the subscriber process.
It only affects which samples are made available via the data reader.

.. _qos-ownership:
.. _quality_of_service--ownership:

Ownership QoS
=============

..
    Sect<3.2.22>

The ownership QoS policy controls whether more than one :term:`DataWriter` is able to write :term:`sample`\s for the same :term:`instance`.

.. important::

  This policy is :ref:`immutable <qos-changing>` and affects :ref:`association <qos-ownership-association>`.

  IDL:

  .. code-block:: omg-idl

    enum OwnershipQosPolicyKind {
      SHARED_OWNERSHIP_QOS,
      EXCLUSIVE_OWNERSHIP_QOS
    };

    struct OwnershipQosPolicy {
      OwnershipQosPolicyKind kind;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`Topic`, :term:`DataWriter`, and :term:`DataReader`

       - ``kind``

       - ``SHARED_OWNERSHIP_QOS``

  Specification: :omgspec:`dds:2.2.3.9 OWNERSHIP`

If the kind member is set to ``SHARED_OWNERSHIP_QOS``, more than one data writer is allowed to update the same instance.
If the kind member is set to ``EXCLUSIVE_OWNERSHIP_QOS``, only one data writer is allowed to update a given instance (i.e., the data writer is considered to be the *owner* of the instance) and associated :term:`DataReader`\s will only see samples written by that data writer.
The owner of the instance is determined by value of the :ref:`qos-ownership-strength` policy; the data writer with the highest value of strength is considered the owner of the instance.
Other factors may also influence ownership, such as whether the data writer with the highest strength is "alive" (as defined by the :ref:`qos-liveliness` policy) and has not violated its offered publication deadline constraints (as defined by the :ref:`qos-deadline` policy).

.. _qos-ownership-association:

For a reader and writer to associate, the ``kind`` must be the same.

.. _qos-ownership-strength:
.. _quality_of_service--ownership-strength:

Ownership Strength QoS
======================

..
    Sect<3.2.23>

The ownership strength QoS policy is used in conjunction with the :ref:`qos-ownership` ``EXCLUSIVE_OWNERSHIP_QOS`` policy on :term:`DataWriter`\s.

.. important::

  This policy is :ref:`mutable <qos-changing>` and does not affect association.

  IDL:

  .. code-block:: omg-idl

    struct OwnershipStrengthQosPolicy {
      long value;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`DataWriter`

       - ``value``

       - ``0``

  Specification: :omgspec:`dds:2.2.3.10 OWNERSHIP_STRENGTH`

The ``value`` member is used to determine which data writer is the *owner* of the :term:`instance`.
The default value is zero.

.. _qos-property:

Property QoS
============

The property QoS policy contains sequences of key-value pairs for the :term:`DomainParticipant`.

.. important::

  This policy is :ref:`mutable <qos-changing>`, but updates to properties after creating the participant might not an effect.
  This policy affects association indirectly through security.

  IDL:

  .. code-block:: omg-idl

    struct Property_t {
      string name;
      string value;
      boolean propagate;
    };
    typedef sequence<Property_t> PropertySeq;

    struct BinaryProperty_t {
      string name;
      OctetSeq value;
      boolean propagate;
    };
    typedef sequence<BinaryProperty_t> BinaryPropertySeq;

    struct PropertyQosPolicy {
      PropertySeq value;
      BinaryPropertySeq binary_value;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`DomainParticipant`

       - ``value``

         ``binary_value``

       - (empty sequence)

         (empty sequence)

  Specification: :omgspec:`sec:7.2.5 PropertyQosPolicy, DomainParticipantQos, DataWriterQos, and DataReaderQos`

Right now these are only used for :ref:`dds_security--dds-security-configuration-via-propertyqospolicy`.

.. _qos-data-representation:

Data Representation QoS
=======================

The data representation QoS policy defines how a :term:`DataWriter` encodes :term:`sample`\s and what encodings a :term:`DataReader` will accept.
This XTypes concept is explained in detail in :ref:`xtypes--data-representation`.

.. important::

  This policy is :ref:`immutable <qos-changing>` and affects :ref:`association <qos-data-representation-association>`.

  IDL:

  .. code-block:: omg-idl

    module DDS {
      const DataRepresentationId_t XCDR_DATA_REPRESENTATION = 0;
      const DataRepresentationId_t XML_DATA_REPRESENTATION = 1;
      const DataRepresentationId_t XCDR2_DATA_REPRESENTATION = 2;

      typedef sequence<DataRepresentationId_t> DataRepresentationIdSeq;

      struct DataRepresentationQosPolicy {
        DataRepresentationIdSeq value;
      };
    };

    module OpenDDS {
      module DCPS {
        const DDS::DataRepresentationId_t UNALIGNED_CDR_DATA_REPRESENTATION = -12140;
      };
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`DataWriter` using the :ref:`rtps-udp-transport`

       - ``value``

       - (empty sequence) -- interpreted as a sequence containing ``XCDR2_DATA_REPRESENTATION``

     * - :term:`DataReader` using the :ref:`rtps-udp-transport`

       - ``value``

       - (empty sequence) -- interpreted as a sequence containing ``XCDR_DATA_REPRESENTATION`` and ``XCDR2_DATA_REPRESENTATION``

     * - :term:`DataWriter` using other transports

       - ``value``

       - (empty sequence) -- interpreted as a sequence containing ``UNALIGNED_CDR_DATA_REPRESENTATION``

     * - :term:`DataReader` using other transports

       - ``value``

       - (empty sequence) -- interpreted as a sequence containing ``XCDR_DATA_REPRESENTATION``, ``XCDR2_DATA_REPRESENTATION``, and ``UNALIGNED_CDR_DATA_REPRESENTATION``

  Specification: :omgspec:`xtypes:7.6.3.1 Data Representation QoS Policy`

.. warning::

  The default interpretation of ``value`` is OpenDDS-specific as of XTypes 1.3.
  The XTypes specification v1.3 specifies that it should be interpreted as a sequence containing ``XCDR_DATA_REPRESENTATION``.
  This is because OpenDDS defaults to XCDR2 instead of XCDR1 when using the :ref:`rtps-udp-transport`, the use of unaligned CDR, and the desire to have readers be as compatible as possible by default.
  See :ref:`xtypes--data-representation` for details.

.. _qos-data-representation-association:

For a reader and writer to associate, the first value in the writer's effective ``value`` must be contained in the reader's effective ``value``.
Other items values in a writer's ``value`` are ignored.

.. _qos-type-consistency-enforcement:

Type Consistency Enforcement QoS
================================

The type consistency enforcement QoS policy lets the application fine-tune details of how :term:`topic type`\s may differ between :term:`DataWriter`\s and :term:`DataReader`\s.
This XTypes concept is explained in detail in :ref:`xtypes--type-consistency-enforcement`.

.. important::

  OpenDDS only supports this with :ref:`rtps-disc`.

  This policy is :ref:`immutable <qos-changing>` and affects :ref:`association <xtypes--type-consistency-enforcement>`.

  IDL:

  .. code-block:: omg-idl

    enum TypeConsistencyKind {
      DISALLOW_TYPE_COERCION,
      ALLOW_TYPE_COERCION
    };

    struct TypeConsistencyEnforcementQosPolicy {
      TypeConsistencyEnforcementQosPolicyKind_t kind;
      boolean ignore_sequence_bounds;
      boolean ignore_string_bounds;
      boolean ignore_member_names;
      boolean prevent_type_widening;
      boolean force_type_validation;
    };

  .. list-table::
     :header-rows: 1

     * - :term:`Applicable entities <Entity>`

       - Members

       - :ref:`Default values <qos-defaults>`

     * - :term:`DataReader`

       - ``kind``

         ``ignore_sequence_bounds``

         ``ignore_string_bounds``

         ``ignore_member_names``

         ``prevent_type_widening``

         ``force_type_validation``

       - ``ALLOW_TYPE_COERCION``

         ``true``

         ``true``

         ``false``

         ``false``

         ``false``

  Specification: :omgspec:`xtypes:7.6.3.4 Type Consistency Enforcement QoS Policy`

.. attention::

  OpenDDS only supports ``ignore_member_names``.
  All other members should be left at their default values.

``ignore_member_names`` defaults to ``false`` so member names (along with member IDs, see :ref:`xtypes--member-id-assignment`) are significant for type compatibility.
Changing this to ``true`` means that only member IDs are used for type compatibility.

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
                                   0,   // No listener
                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

This code creates a publisher with the following qualities:

* :ref:`qos-history` set to "keep all"

* :ref:`qos-reliability` set to "reliable" with a maximum blocking time of 10 seconds

* The maximum :term:`sample`\s per :term:`instance` :ref:`qos-resource-limits` set to 100

This means that when 100 samples are waiting to be delivered, the writer can block up to 10 seconds before returning an error code.
These same QoS settings on the data reader side would mean that up to 100 unread samples are queued by the framework before any are rejected.
Rejected samples are dropped and the :ref:`SampleRejectedStatus <conditions_and_listeners--sample-rejected-status>` is updated.

.. rubric:: Footnotes

.. [#footnote1]

   For OpenDDS versions, up to 2.0, the default reliability kind for data writers is best effort.
   For versions 2.0.1 and later, this is changed to reliable (to conform to the DDS specification).

.. [#user_data_ex]

  This is an example that was given by the DDS specification.
  If you actually need to authenticate remote participants in a secure way, it's highly recommended to use :ref:`sec`.
