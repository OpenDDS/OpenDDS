.. _built_in_topics:
.. _bit:

###############
Built-in Topics
###############

..
    Sect<6>

.. _built_in_topics--introduction:

************
Introduction
************

..
    Sect<6.1>

In OpenDDS, built-in topics are created and published by default to exchange information about DDS participants operating in the deployment.
When OpenDDS is :ref:`inforepo-disc`, the built-in topics are published by this service.
For :ref:`rtps-disc`, the internal OpenDDS implementation instantiated in a process populates the caches of the built-In topic DataReaders.

The IDL struct ``BuiltinTopicKey_t`` is used by the built-in topics.
This structure contains an array of 16 octets (bytes) which corresponds to an InfoRepo identifier or a DDSI-RTPS GUID.

.. _built_in_topics--built-in-topics-for-dcpsinforepo-configuration:

**********************************************
Built-in Topics for DCPSInfoRepo Configuration
**********************************************

..
    Sect<6.2>

When starting the ``DCPSInfoRepo`` a command line option of ``-NOBITS`` may be used to suppress publication of built-in topics.

Four separate topics are defined for each domain.
Each is dedicated to a particular entity (domain participant :ref:`built_in_topics--dcpsparticipant-topic`, topic :ref:`built_in_topics--dcpsparticipant-topic`, data writer :ref:`built_in_topics--dcpspublication-topic`, data reader :ref:`built_in_topics--dcpssubscription-topic`) and publishes instances describing the state for each entity in the domain.

Subscriptions to built-in topics are automatically created for each domain participant.
A participant's support for built-in topics can be toggled via the :cfg:prop:`DCPSBit` configuration option.
To view the built-in topic data, simply obtain the built-in Subscriber and then use it to access the Data Reader for the built-in topic of interest.
The Data Reader can then be used like any other Data Reader.

See :ref:`built_in_topics--built-in-topic-subscription-example` for an example showing how to read from a built-in topic.

If you are not planning on using built-in topics in your application, you can configure OpenDDS to remove built-in topic support at build time.
Doing so can reduce the footprint of the core DDS library by up to 30%.
See :ref:`building--disabling-the-building-of-built-in-topic-support` for information on disabling built-in topic support.

.. _built_in_topics--dcpsparticipant-topic:

*********************
DCPSParticipant Topic
*********************

..
    Sect<6.3>

The ``DCPSParticipant`` topic publishes information about the Domain Participants of the Domain.
Here is the IDL that defines the structure published for this topic:

.. code-block:: omg-idl

        struct ParticipantBuiltinTopicData {
          BuiltinTopicKey_t key;
          UserDataQosPolicy user_data;
        };

Each Domain Participant is defined by a unique key and is its own instance within this topic.

.. _built_in_topics--dcpstopic-topic:

***************
DCPSTopic Topic
***************

..
    Sect<6.4>

.. note:: OpenDDS does not support this built-in topic when configured for :ref:`rtps-disc`.

The ``DCPSTopic`` topic publishes information about the topics in the domain.
Here is the IDL that defines the structure published for this topic:

.. code-block:: omg-idl

        struct TopicBuiltinTopicData {
          BuiltinTopicKey_t key;
          string name;
          string type_name;
          DurabilityQosPolicy durability;
          QosPolicy deadline;
          LatencyBudgetQosPolicy latency_budget;
          LivelinessQosPolicy liveliness;
          ReliabilityQosPolicy reliability;
          TransportPriorityQosPolicy transport_priority;
          LifespanQosPolicy lifespan;
          DestinationOrderQosPolicy destination_order;
          HistoryQosPolicy history;
          ResourceLimitsQosPolicy resource_limits;
          OwnershipQosPolicy ownership;
          TopicDataQosPolicy topic_data;
        };

Each topic is identified by a unique key and is its own instance within this built-in topic.
The members above identify the name of the topic, the name of the topic type, and the set of QoS policies for that topic.

.. _built_in_topics--dcpspublication-topic:

*********************
DCPSPublication Topic
*********************

..
    Sect<6.5>

The ``DCPSPublication`` topic publishes information about the Data Writers in the Domain.
Here is the IDL that defines the structure published for this topic:

.. code-block:: omg-idl

        struct PublicationBuiltinTopicData {
          BuiltinTopicKey_t key;
          BuiltinTopicKey_t participant_key;
          string topic_name;
          string type_name;
          DurabilityQosPolicy durability;
          DeadlineQosPolicy deadline;
          LatencyBudgetQosPolicy latency_budget;
          LivelinessQosPolicy liveliness;
          ReliabilityQosPolicy reliability;
          LifespanQosPolicy lifespan;
          UserDataQosPolicy user_data;
          OwnershipStrengthQosPolicy ownership_strength;
          PresentationQosPolicy presentation;
          PartitionQosPolicy partition;
          TopicDataQosPolicy topic_data;
          GroupDataQosPolicy group_data;
        };

Each Data Writer is assigned a unique key when it is created and defines its own instance within this topic.
The fields above identify the Domain Participant (via its key) that the Data Writer belongs to, the topic name and type, and the various QoS policies applied to the Data Writer.

.. _built_in_topics--dcpssubscription-topic:

**********************
DCPSSubscription Topic
**********************

..
    Sect<6.6>

The ``DCPSSubscription`` topic publishes information about the Data Readers in the Domain.
Here is the IDL that defines the structure published for this topic:

.. code-block:: omg-idl

        struct SubscriptionBuiltinTopicData {
          BuiltinTopicKey_t key;
          BuiltinTopicKey_t participant_key;
          string topic_name;
          string type_name;
          DurabilityQosPolicy durability;
          DeadlineQosPolicy deadline;
          LatencyBudgetQosPolicy latency_budget;
          LivelinessQosPolicy liveliness;
          ReliabilityQosPolicy reliability;
          DestinationOrderQosPolicy destination_order;
          UserDataQosPolicy user_data;
          TimeBasedFilterQosPolicy time_based_filter;
          PresentationQosPolicy presentation;
          PartitionQosPolicy partition;
          TopicDataQosPolicy topic_data;
          GroupDataQosPolicy group_data;
        };

Each Data Reader is assigned a unique key when it is created and defines its own instance within this topic.
The fields above identify the Domain Participant (via its key) that the Data Reader belongs to, the topic name and type, and the various QoS policies applied to the Data Reader.

.. _built_in_topics--built-in-topic-subscription-example:

***********************************
Built-in Topic Subscription Example
***********************************

..
    Sect<6.7>

The following code uses a domain participant to get the built-in subscriber.
It then uses the subscriber to get the Data Reader for the ``DCPSParticipant`` topic and subsequently reads samples for that reader.

.. code-block:: cpp

        Subscriber_var bit_subscriber = participant->get_builtin_subscriber();
        DDS::DataReader_var dr =
          bit_subscriber->lookup_datareader(BUILT_IN_PARTICIPANT_TOPIC);
        DDS::ParticipantBuiltinTopicDataDataReader_var part_dr =
          DDS::ParticipantBuiltinTopicDataDataReader::_narrow(dr);

        DDS::ParticipantBuiltinTopicDataSeq part_data;
        DDS::SampleInfoSeq infos;
        DDS::ReturnCode_t ret = part_dr->read(part_data, infos, 20,
                                              DDS::ANY_SAMPLE_STATE,
                                              DDS::ANY_VIEW_STATE,
                                              DDS::ANY_INSTANCE_STATE);

        // Check return status and read the participant data

The code for the other built-in topics is similar.

.. _built_in_topics--opendds-specific-built-in-topics:

********************************
OpenDDS-specific Built-in Topics
********************************

..
    Sect<6.8>

.. _built_in_topics--openddsparticipantlocation-topic:

OpenDDSParticipantLocation Topic
================================

..
    Sect<6.8.1>

The built-in topic ``OpenDDSParticipantLocation`` is published by the DDSI-RTPS discovery implementation to give applications visibility into the details of how each remote participant is connected over the network.
If the RtpsRelay (:ref:`internet_enabled_rtps--the-rtpsrelay`) and/or IETF ICE (:ref:`internet_enabled_rtps--interactive-connectivity-establishment-ice-for-rtps`) are enabled, their usage is reflected in the OpenDDSParticipantLocation topic data.
Instances of this built-in topic are published before participant discovery is complete so that applications can be notified that discovery is in progress.
The topic type ParticipantLocationBuiltinTopicData is defined in :ghfile:`dds/OpenddsDcpsExt.idl` in the ``OpenDDS::DCPS`` module:

* ``guid`` (key) -- The GUID of the remote participant.
  Also, a key into the DCPSParticipant topic.

* ``location`` -- A bit-mask indicating which fields are populated.

* ``change_mask`` -- A bit-mask indicating which fields changed.

* ``local_addr`` -- SPDP address of the remote participant for a local connection.

* ``local_timestamp`` -- Time that ``local_addr`` was set.

* ``ice_addr`` -- SPDP address of the remote participant for an ICE connection.

* ``ice_timestamp`` -- Time that ``ice_addr`` was set.

* ``relay_addr`` -- SPDP address of the remote participant using the RtpsRelay.

* ``relay_timestamp`` -- Time that ``relay_addr`` was set.

* ``local6_addr``, ``local6_timestamp``, ``ice6_addr``, ``ice6_timestamp``, ``relay6_addr``, and ``relay6_timestamp`` -- Are the IPV6 equivalents.

* ``lease_duration`` -- The remote participant's :cfg:prop:`[rtps_discovery]LeaseDuration`

* ``user_tag`` -- The remote participant's :cfg:prop:`[rtps_discovery]SpdpUserTag`

.. _built_in_topics--openddsconnectionrecord-topic:

OpenDDSConnectionRecord Topic
=============================

..
    Sect<6.8.2>

The built-in topic ``OpenDDSConnectionRecord`` is published by the DDSI-RTPS discovery implementation and RTPS_UDP transport implementation to give applications visibility into the details of a participant's connection to an RtpsRelay instance.
Security must be enabled in the build of OpenDDS (:ref:`dds_security--building-opendds-with-security-enabled`) to use this topic.

The topic type ConnectionRecord is defined in :ghfile:`dds/OpenddsDcpsExt.idl` in the ``OpenDDS::DCPS`` module:

* ``guid`` (key) -- The GUID of the remote participant.
  Also, a key into the DCPSParticipant topic.

* ``address`` (key) -- The address of the remote participant.

* ``protocol`` (key) -- The method used to determine connectivity.
  Currently, "RtpsRelay:STUN" is the only supported protocol.

* ``latency`` -- A measured round-trip latency for protocols that support it.

.. _built_in_topics--openddsinternalthread-topic:

OpenDDSInternalThread Topic
===========================

..
    Sect<6.8.3>

The built-in topic ``OpenDDSInternalThread`` is published when OpenDDS is configured with :cfg:prop:`DCPSThreadStatusInterval`.
When enabled, the DataReader for this built-in topic will report the status of threads created and managed by OpenDDS within the current process.
The timestamp associated with samples can be used to determine the health (responsiveness) of the thread.

The topic type InternalThreadBuiltinTopicData is defined in :ghfile:`dds/OpenddsDcpsExt.idl` in the ``OpenDDS::DCPS`` module:

* ``thread_id`` (key) -- A string identifier for the thread.

* ``utilization`` -- Estimated utilization of this thread (0.0-1.0).

* ``monotonic_timestamp`` -- Time of most recent update (monotonic clock).  The SampleInfo's ``source_timestamp`` has the timestamp on the system clock.
