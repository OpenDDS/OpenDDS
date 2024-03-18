.. _conditions_and_listeners:

########################
Conditions and Listeners
########################

..
    Sect<4>

.. _conditions_and_listeners--introduction:

************
Introduction
************

..
    Sect<4.1>

The DDS specification defines two separate mechanisms for notifying applications of DCPS communication status changes.
Most of the status types define a structure that contains information related to the change of status and can be detected by the application using conditions or listeners.
The different status types are described in :ref:`conditions_and_listeners--communication-status-types`.

Each :term:`entity` type defines its own corresponding listener interface.
Applications can implement this interface and then attach their listener implementation to the entity.
Each listener interface contains an operation for each status that can be reported for that entity.
The listener is asynchronously called back with the appropriate operation whenever a qualifying status change occurs.
Details of the different listener types are discussed in :ref:`conditions_and_listeners--listeners`.

Conditions are used in conjunction with Wait Sets to let applications synchronously wait on events.
The basic usage pattern for conditions involves creating the condition objects, attaching them to a wait set, and then waiting on the wait set until one of the conditions is triggered.
The result of wait tells the application which conditions were triggered, allowing the application to take the appropriate actions to get the corresponding status information.
Conditions are described in greater detail in :ref:`conditions_and_listeners--conditions`.

.. _conditions_and_listeners--communication-status-types:

**************************
Communication Status Types
**************************

..
    Sect<4.2>

Each status type is associated with a particular :term:`entity` type.
This section is organized by the entity types, with the corresponding statuses described in subsections under the associated entity type.

Most of the statuses below are plain communication statuses.
The exceptions are ``DATA_ON_READERS`` and ``DATA_AVAILABLE`` which are read statuses.
Plain communication statuses define an IDL data structure.
Their corresponding section below describes this structure and its fields.
The read statuses are simple notifications to the application which then reads or takes the samples as desired.

Incremental values in the status data structure report a change since the last time the status was accessed.
A status is considered accessed when a listener is called for that status or the status is read from its entity.

Fields in the status data structure with a type of ``InstanceHandle_t`` identify an entity by the instance handle used for that entity in the :term:`built-in topics`.

.. _conditions_and_listeners--topic-status-types:

Topic Status Types
==================

..
    Sect<4.2.1>

.. _conditions_and_listeners--inconsistent-topic-status:

Inconsistent Topic Status
-------------------------

..
    Sect<4.2.1.1>

The ``INCONSISTENT_TOPIC`` status indicates that the :term:`topic` being registered has different characteristics than an existing topic with the same name.
Typically, the existing topic may have a different type associated with it.
The IDL associated with the Inconsistent Topic Status is listed below:

.. code-block:: omg-idl

    struct InconsistentTopicStatus {
      long total_count;
      long total_count_change;
    };

The ``total_count`` value is the cumulative count of topics that have been reported as inconsistent.
The ``total_count_change`` value is the incremental count of inconsistent topics since the last time this status was accessed.

.. _conditions_and_listeners--subscriber-status-types:

Subscriber Status Types
=======================

..
    Sect<4.2.2>

.. _conditions_and_listeners--data-on-readers-status:

Data On Readers Status
----------------------

..
    Sect<4.2.2.1>

The ``DATA_ON_READERS`` status indicates that new data is available on some of the data readers associated with the subscriber.
This status is considered a read status and does not define an IDL structure.
Applications receiving this status can call ``get_datareaders()`` on the subscriber to get the set of data readers with data available.

.. _conditions_and_listeners--data-reader-status-types:

Data Reader Status Types
========================

..
    Sect<4.2.3>

.. _conditions_and_listeners--sample-rejected-status:

Sample Rejected Status
----------------------

..
    Sect<4.2.3.1>

The ``SAMPLE_REJECTED`` status indicates that a sample received by the data reader has been rejected.
The IDL associated with the Sample Rejected Status is listed below:

.. code-block:: omg-idl

    enum SampleRejectedStatusKind {
      NOT_REJECTED,
      REJECTED_BY_INSTANCES_LIMIT,
      REJECTED_BY_SAMPLES_LIMIT,
      REJECTED_BY_SAMPLES_PER_INSTANCE_LIMIT
    };

    struct SampleRejectedStatus {
      long total_count;
      long total_count_change;
      SampleRejectedStatusKind last_reason;
      InstanceHandle_t last_instance_handle;
    };

The ``total_count`` value is the cumulative count of samples that have been reported as rejected.
The ``total_count_change`` value is the incremental count of rejected samples since the last time this status was accessed.
The ``last_reason`` value is the reason the most recently rejected sample was rejected.
The ``last_instance_handle`` value indicates the instance of the last rejected sample.

.. _conditions_and_listeners--liveliness-changed-status:

Liveliness Changed Status
-------------------------

..
    Sect<4.2.3.2>

The ``LIVELINESS_CHANGED`` status indicates that there have been :ref:`liveliness changes <qos-liveliness>` for one or more data writers that are publishing instances for this data reader.
The IDL associated with the Liveliness Changed Status is listed below:

.. code-block:: omg-idl

    struct LivelinessChangedStatus {
      long alive_count;
      long not_alive_count;
      long alive_count_change;
      long not_alive_count_change;
      InstanceHandle_t last_publication_handle;
    };

The ``alive_count`` value is the total number of data writers currently active on the :term:`topic` this data reader is reading.
The ``not_alive_count`` value is the total number of data writers writing to the data reader's topic that are no longer asserting their liveliness.
The ``alive_count_change`` value is the change in the alive count since the last time the status was accessed.
The ``not_alive_count_change`` value is the change in the not alive count since the last time the status was accessed.
The ``last_publication_handle`` is the handle of the last data writer whose liveliness has changed.

.. _conditions_and_listeners--requested-deadline-missed-status:

Requested Deadline Missed Status
--------------------------------

..
    Sect<4.2.3.3>

The ``REQUESTED_DEADLINE_MISSED`` status indicates that the deadline requested via the :ref:`qos-deadline` policy was not respected for a specific instance.
The IDL associated with the Requested Deadline Missed Status is listed below:

.. code-block:: omg-idl

    struct RequestedDeadlineMissedStatus {
      long total_count;
      long total_count_change;
      InstanceHandle_t last_instance_handle;
    };

The ``total_count`` value is the cumulative count of missed requested deadlines that have been reported.
The ``total_count_change`` value is the incremental count of missed requested deadlines since the last time this status was accessed.
The ``last_instance_handle`` value indicates the instance of the last missed deadline.

.. _conditions_and_listeners--requested-incompatible-qos-status:

Requested Incompatible QoS Status
---------------------------------

..
    Sect<4.2.3.4>

The ``REQUESTED_INCOMPATIBLE_QOS`` status indicates that one or more :term:`qos` policy values that were requested by a local :term:`DataReader` were incompatible with what was offered by a :term:`DataWriter`.
See :ref:`qos-changing` for details.
The IDL associated with the Requested Incompatible QoS Status is listed below:

.. code-block:: omg-idl

    struct QosPolicyCount {
      QosPolicyId_t policy_id;
      long count;
    };

    typedef sequence<QosPolicyCount> QosPolicyCountSeq;

    struct RequestedIncompatibleQosStatus {
      long total_count;
      long total_count_change;
      QosPolicyId_t last_policy_id;
      QosPolicyCountSeq policies;
    };

The ``total_count`` value is the cumulative count of times data writers with incompatible QoS have been reported.
The ``total_count_change`` value is the incremental count of incompatible data writers since the last time this status was accessed.
The ``last_policy_id`` value identifies one of the QoS policies that was incompatible in the last incompatibility detected.
The policies value is a sequence of values that indicates the total number of incompatibilities that have been detected for each QoS policy.

.. _conditions_and_listeners--data-available-status:

Data Available Status
---------------------

..
    Sect<4.2.3.5>

The ``DATA_AVAILABLE`` status indicates that samples are available on the data writer.
This status is considered a read status and does not define an IDL structure.
Applications receiving this status can use the various take and read operations on the data reader to retrieve the data.

.. _conditions_and_listeners--sample-lost-status:

Sample Lost Status
------------------

..
    Sect<4.2.3.6>

The ``SAMPLE_LOST`` status indicates that a sample has been lost and never received by the data reader.
The IDL associated with the Sample Lost Status is listed below:

.. code-block:: omg-idl

    struct SampleLostStatus {
      long total_count;
      long total_count_change;
    };

The ``total_count`` value is the cumulative count of samples reported as lost.
The ``total_count_change`` value is the incremental count of lost samples since the last time this status was accessed.

.. _conditions_and_listeners--subscription-matched-status:

Subscription Matched Status
---------------------------

..
    Sect<4.2.3.7>

The ``SUBSCRIPTION_MATCHED`` status indicates that either a compatible data writer has been matched or a previously matched data writer has ceased to be matched.
The IDL associated with the Subscription Matched Status is listed below:

.. code-block:: omg-idl

    struct SubscriptionMatchedStatus {
      long total_count;
      long total_count_change;
      long current_count;
      long current_count_change;
      InstanceHandle_t last_publication_handle;
    };

The ``total_count`` value is the cumulative count of data writers that have compatibly matched this data reader.
The ``total_count_change`` value is the incremental change in the total count since the last time this status was accessed.
The ``current_count`` value is the current number of data writers matched to this data reader.
The ``current_count_change`` value is the change in the current count since the last time this status was accessed.
The ``last_publication_handle`` value is a handle for the last data writer matched.

.. _conditions_and_listeners--data-writer-status-types:

Data Writer Status Types
========================

..
    Sect<4.2.4>

.. _conditions_and_listeners--liveliness-lost-status:

Liveliness Lost Status
----------------------

..
    Sect<4.2.4.1>

The ``LIVELINESS_LOST`` status indicates that the liveliness that the data writer committed through its Liveliness QoS has not been respected.
This means that any connected data readers will consider this data writer no longer active.The IDL associated with the Liveliness Lost Status is listed below:

.. code-block:: omg-idl

    struct LivelinessLostStatus {
      long total_count;
      long total_count_change;
    };

The ``total_count`` value is the cumulative count of times that an alive data writer has become not alive.
The ``total_count_change`` value is the incremental change in the total count since the last time this status was accessed.

.. _conditions_and_listeners--offered-deadline-missed-status:

Offered Deadline Missed Status
------------------------------

..
    Sect<4.2.4.2>

The ``OFFERED_DEADLINE_MISSED`` status indicates that the :ref:`deadline <qos-deadline>` offered by the data writer has been missed for one or more instances.
The IDL associated with the Offered Deadline Missed Status is listed below:

.. code-block:: omg-idl

    struct OfferedDeadlineMissedStatus {
      long total_count;
      long total_count_change;
      InstanceHandle_t last_instance_handle;
    };

The ``total_count`` value is the cumulative count of times that deadlines have been missed for an instance.
The ``total_count_change`` value is the incremental change in the total count since the last time this status was accessed.
The ``last_instance_handle`` value indicates the last instance that has missed a deadline.

.. _conditions_and_listeners--offered-incompatible-qos-status:

Offered Incompatible QoS Status
-------------------------------

..
    Sect<4.2.4.3>

The ``OFFERED_INCOMPATIBLE_QOS`` status indicates that one or more :term:`qos` policy values offered by a local :term:`DataWriter` were incompatible with what was requested by a :term:`DataReader`.
See :ref:`qos-changing` for details.
The IDL associated with the Offered Incompatible QoS Status is listed below:

.. code-block:: omg-idl

    struct QosPolicyCount {
      QosPolicyId_t policy_id;
      long count;
    };
    typedef sequence<QosPolicyCount> QosPolicyCountSeq;

    struct OfferedIncompatibleQosStatus {
      long total_count;
      long total_count_change;
      QosPolicyId_t last_policy_id;
      QosPolicyCountSeq policies;
    };

The ``total_count`` value is the cumulative count of times that data readers with incompatible QoS have been found.
The ``total_count_change`` value is the incremental change in the total count since the last time this status was accessed.
The ``last_policy_id`` value identifies one of the QoS policies that was incompatible in the last incompatibility detected.
The ``policies`` value is a sequence of values that indicates the total number of incompatibilities that have been detected for each QoS policy.

.. _conditions_and_listeners--publication-matched-status:

Publication Matched Status
--------------------------

..
    Sect<4.2.4.4>

The ``PUBLICATION_MATCHED`` status indicates that either a compatible data reader has been matched or a previously matched data reader has ceased to be matched.
The IDL associated with the Publication Matched Status is listed below:

.. code-block:: omg-idl

    struct PublicationMatchedStatus {
      long total_count;
      long total_count_change;
      long current_count;
      long current_count_change;
      InstanceHandle_t last_subscription_handle;
    };

The ``total_count`` value is the cumulative count of data readers that have compatibly matched this data writer.
The ``total_count_change`` value is the incremental change in the total count since the last time this status was accessed.
The ``current_count`` value is the current number of data readers matched to this data writer.
The ``current_count_change`` value is the change in the current count since the last time this status was accessed.
The ``last_subscription_handle`` value is a handle for the last data reader matched.

.. _conditions_and_listeners--budget-exceeded-status:

Budget Exceeded Status
----------------------

This is an OpenDDS-specific listener extension allows for reporting delays in excess of the :ref:`qos-latency-budget`.
The ``OpenDDS::DCPS::DataReaderListener`` interface has an additional operation for notification that samples were received with a measured transport delay greater than the latency budget policy duration.
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
    : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataReaderListener>
  {
    void on_budget_exceeded(
      DDS::DataReader* reader,
      const OpenDDS::DCPS::BudgetExceededStatus& status)
    {
    }

Then you must provide a non-null implementation for the ``on_budget_exceeded()`` operation.
Note that you will need to provide empty implementations for the following extended operations as well:

.. code-block:: cpp

  void on_subscription_disconnected(
    DDS::DataReader* reader,
    const OpenDDS::DCPS::SubscriptionDisconnectedStatus& status)
  {
  }

  void on_subscription_reconnected(
    DDS::DataReader* reader,
    const OpenDDS::DCPS::SubscriptionReconnectedStatus& status)
  {
  }

  void on_subscription_lost(
    DDS::DataReader* reader,
    const OpenDDS::DCPS::SubscriptionLostStatus& status)
  {
  }

OpenDDS also makes the summary latency statistics available via an extended interface of the data reader.
This extended interface is located in the ``OpenDDS::DCPS`` module and the IDL is defined as:

.. code-block:: omg-idl

      struct LatencyStatistics {
        GUID_t        publication;
        unsigned long n;
        double        maximum;
        double        minimum;
        double        mean;
        double        variance;
      };

      typedef sequence<LatencyStatistics> LatencyStatisticsSeq;

      local interface DataReaderEx : DDS::DataReader {
        /// Obtain a sequence of statistics summaries.
        void get_latency_stats(inout LatencyStatisticsSeq stats);

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
      for (unsigned long i = 0; i < stats.length(); ++i) {
        std::cout << "stats[" << i << "]:" << std::endl;
        std::cout << "         n = " << stats[i].n << std::endl;
        std::cout << "       max = " << stats[i].maximum << std::endl;
        std::cout << "       min = " << stats[i].minimum << std::endl;
        std::cout << "      mean = " << stats[i].mean << std::endl;
        std::cout << "  variance = " << stats[i].variance << std::endl;
      }

.. _conditions_and_listeners--listeners:

*********
Listeners
*********

..
    Sect<4.3>

Each entity defines its own listener interface based on the statuses it can report.
Any entity's listener interface also inherits from the listeners of its owned entities, allowing it to handle statuses for owned entities as well.
For example, a subscriber listener directly defines an operation to handle Data On Readers statuses and inherits from the data reader listener as well.

Each status operation takes the general form of ``on_<status_name>(<entity>, <status_struct>)``, where ``<status_name>`` is the name of the status being reported, ``<entity>`` is a reference to the entity the status is reported for, and ``<status_struct>`` is the structure with details of the status.
Read statuses omit the second parameter.
For example, here is the operation for the Sample Lost status:

.. code-block:: cpp

      void on_sample_lost(in DataReader the_reader, in SampleLostStatus status);

Listeners can either be passed to the factory function used to create their entity or explicitly set by calling ``set_listener()`` on the entity after it is created.
Both of these functions also take a status mask as a parameter.
The mask indicates which statuses are enabled in that listener.
Mask bit values for each status are defined in DdsDcpsInfrastructure.idl:

.. code-block:: omg-idl

    module DDS {
      typedef unsigned long StatusKind;
      typedef unsigned long StatusMask; // bit-mask StatusKind

      const StatusKind INCONSISTENT_TOPIC_STATUS        = 0x0001 << 0;
      const StatusKind OFFERED_DEADLINE_MISSED_STATUS   = 0x0001 << 1;
      const StatusKind REQUESTED_DEADLINE_MISSED_STATUS = 0x0001 << 2;
      const StatusKind OFFERED_INCOMPATIBLE_QOS_STATUS  = 0x0001 << 5;
      const StatusKind REQUESTED_INCOMPATIBLE_QOS_STATUS= 0x0001 << 6;
      const StatusKind SAMPLE_LOST_STATUS               = 0x0001 << 7;
      const StatusKind SAMPLE_REJECTED_STATUS           = 0x0001 << 8;
      const StatusKind DATA_ON_READERS_STATUS           = 0x0001 << 9;
      const StatusKind DATA_AVAILABLE_STATUS            = 0x0001 << 10;
      const StatusKind LIVELINESS_LOST_STATUS           = 0x0001 << 11;
      const StatusKind LIVELINESS_CHANGED_STATUS        = 0x0001 << 12;
      const StatusKind PUBLICATION_MATCHED_STATUS       = 0x0001 << 13;
      const StatusKind SUBSCRIPTION_MATCHED_STATUS      = 0x0001 << 14;
    };

Simply do a bit-wise "or" of the desired status bits to construct a mask for your listener.
Here is an example of attaching a listener to a data reader (for just Data Available statuses):

.. code-block:: cpp

        DDS::DataReaderListener_var listener (new DataReaderListenerImpl);
        // Create the Datareader
        DDS::DataReader_var dr = sub->create_datareader(
          topic,
          DATAREADER_QOS_DEFAULT,
          listener,
          DDS::DATA_AVAILABLE_STATUS);

Here is an example showing how to change the listener using ``set_listener()``:

.. code-block:: cpp

      dr->set_listener(listener,
                       DDS::DATA_AVAILABLE_STATUS | DDS::LIVELINESS_CHANGED_STATUS);

When a plain communication status changes, OpenDDS invokes the most specific relevant listener operation.
This means, for example, that a data reader's listener would take precedence over the subscriber's listener for statuses related to the data reader.

A common "gotcha" when using ``set_listener`` is that the listener is not invoked immediately.
Instead, the listener will be invoked for the next status change.
Consequently, usages of ``set_listener`` should 1) invoke the listener manually after calling ``set_listener`` and 2) ensure that the listener methods are thread safe.

The following sections define the different listener interfaces.
For more details on the individual statuses, see :ref:`conditions_and_listeners--communication-status-types`.

.. _conditions_and_listeners--topic-listener:

Topic Listener
==============

..
    Sect<4.3.1>

.. code-block:: omg-idl

    interface TopicListener : Listener {
      void on_inconsistent_topic(in Topic the_topic,
                                 in InconsistentTopicStatus status);
    };

.. _conditions_and_listeners--data-writer-listener:

Data Writer Listener
====================

..
    Sect<4.3.2>

.. code-block:: omg-idl

    interface DataWriterListener : Listener {
      void on_offered_deadline_missed(in DataWriter writer,
                                      in OfferedDeadlineMissedStatus status);
      void on_offered_incompatible_qos(in DataWriter writer,
                                       in OfferedIncompatibleQosStatus status);
      void on_liveliness_lost(in DataWriter writer,
                              in LivelinessLostStatus status);
      void on_publication_matched(in DataWriter writer,
                                  in PublicationMatchedStatus status);
    };

.. _conditions_and_listeners--publisher-listener:

Publisher Listener
==================

..
    Sect<4.3.3>

.. code-block:: omg-idl

    interface PublisherListener : DataWriterListener {
    };

.. _conditions_and_listeners--data-reader-listener:

Data Reader Listener
====================

..
    Sect<4.3.4>

.. code-block:: omg-idl

    interface DataReaderListener : Listener {
      void on_requested_deadline_missed(in DataReader the_reader,
                                        in RequestedDeadlineMissedStatus status);
      void on_requested_incompatible_qos(in DataReader the_reader,
                                         in RequestedIncompatibleQosStatus status);
      void on_sample_rejected(in DataReader the_reader,
                              in SampleRejectedStatus status);
      void on_liveliness_changed(in DataReader the_reader,
                                 in LivelinessChangedStatus status);
      void on_data_available(in DataReader the_reader);
      void on_subscription_matched(in DataReader the_reader,
                                   in SubscriptionMatchedStatus status);
      void on_sample_lost(in DataReader the_reader,
                          in SampleLostStatus status);
    };

.. _conditions_and_listeners--subscriber-listener:

Subscriber Listener
===================

..
    Sect<4.3.5>

.. code-block:: omg-idl

    interface SubscriberListener : DataReaderListener {
      void on_data_on_readers(in Subscriber the_subscriber);
    };

.. _conditions_and_listeners--domain-participant-listener:

Domain Participant Listener
===========================

..
    Sect<4.3.6>

.. code-block:: omg-idl

    interface DomainParticipantListener : TopicListener,
                                          PublisherListener,
                                          SubscriberListener {
    };

.. _conditions_and_listeners--conditions:

**********
Conditions
**********

..
    Sect<4.4>

The DDS specification defines four types of condition:

* Status Condition

* Read Condition

* Query Condition

* Guard Condition

.. _conditions_and_listeners--status-condition:

Status Condition
================

..
    Sect<4.4.1>

Each entity has a status condition object associated with it and a ``get_statuscondition()`` operation that lets applications access the status condition.
Each condition has a set of enabled statuses that can trigger that condition.
Attaching one or more conditions to a wait set allows application developers to wait on the condition's status set.
Once an enabled status is triggered, the wait call returns from the wait set and the developer can query the relevant status condition on the entity.
Querying the status condition resets the status.

.. _conditions_and_listeners--status-condition-example:

Status Condition Example
------------------------

..
    Sect<4.4.1.1>

This example enables the Offered Incompatible QoS status on a data writer, waits for it, and then queries it when it triggers.
The first step is to get the status condition from the data writer, enable the desired status, and attach it to a wait set:

.. code-block:: cpp

      DDS::StatusCondition_var cond = data_writer->get_statuscondition();
      cond->set_enabled_statuses(DDS::OFFERED_INCOMPATIBLE_QOS_STATUS);

      DDS::WaitSet_var ws = new DDS::WaitSet;
      ws->attach_condition(cond);

Now we can wait ten seconds for the condition:

.. code-block:: cpp

      DDS::ConditionSeq active;
      DDS::Duration_t ten_seconds = {10, 0};
      int result = ws->wait(active, ten_seconds);

The result of this operation is either a timeout or a set of triggered conditions in the active sequence:

.. code-block:: cpp

      if (result == DDS::RETCODE_TIMEOUT) {
        cout << "Wait timed out" << std::endl;
      } else if (result == DDS::RETCODE_OK) {
        DDS::OfferedIncompatibleQosStatus incompatibleStatus;
        data_writer->get_offered_incompatible_qos(incompatibleStatus);
        // Access status fields as desired...
      }

Developers have the option of attaching multiple conditions to a single wait set as well as enabling multiple statuses per condition.

.. _conditions_and_listeners--additional-condition-types:

Additional Condition Types
==========================

..
    Sect<4.4.2>

The DDS specification also defines three other types of conditions: read conditions, query conditions, and guard conditions.
These conditions do not directly involve the processing of statuses but allow the integration of other activities into the condition and wait set mechanisms.
These are other conditions are briefly described here.
For more information see the DDS specification or the OpenDDS tests in :ghfile:`tests/`.

.. _conditions_and_listeners--read-conditions:

Read Conditions
---------------

..
    Sect<4.4.2.1>

Read conditions are created using the data reader and the same masks that are passed to the read and take operations.
When waiting on this condition, it is triggered whenever samples match the specified masks.
Those samples can then be retrieved using the ``read_w_condition()`` and ``take_w_condition()`` operations which take the read condition as a parameter.

.. _conditions_and_listeners--query-conditions:

Query Conditions
----------------

..
    Sect<4.4.2.2>

Query conditions are a specialized form of read conditions that are created with a limited form of an SQL-like query.
This allows applications to filter the data samples that trigger the condition and then are read use the normal read condition mechanisms.
See :ref:`content_subscription_profile--query-condition` for more information about query conditions.

.. _conditions_and_listeners--guard-conditions:

Guard Conditions
----------------

..
    Sect<4.4.2.3>

The guard condition is a simple interface that allows the application to create its own condition object and trigger it when application events (external to OpenDDS) occur.

