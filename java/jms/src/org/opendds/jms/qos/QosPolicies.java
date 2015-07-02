/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.qos;

import DDS.DataReaderQos;
import DDS.DataWriterQos;
import DDS.DeadlineQosPolicy;
import DDS.DestinationOrderQosPolicy;
import DDS.DestinationOrderQosPolicyKind;
import DDS.DomainParticipantQos;
import DDS.DurabilityQosPolicy;
import DDS.DurabilityQosPolicyKind;
import DDS.DurabilityServiceQosPolicy;
import DDS.Duration_t;
import DDS.EntityFactoryQosPolicy;
import DDS.GroupDataQosPolicy;
import DDS.HistoryQosPolicy;
import DDS.HistoryQosPolicyKind;
import DDS.LatencyBudgetQosPolicy;
import DDS.LifespanQosPolicy;
import DDS.LivelinessQosPolicy;
import DDS.LivelinessQosPolicyKind;
import DDS.OwnershipQosPolicy;
import DDS.OwnershipQosPolicyKind;
import DDS.OwnershipStrengthQosPolicy;
import DDS.PartitionQosPolicy;
import DDS.PresentationQosPolicy;
import DDS.PresentationQosPolicyAccessScopeKind;
import DDS.PublisherQos;
import DDS.ReaderDataLifecycleQosPolicy;
import DDS.ReliabilityQosPolicy;
import DDS.ReliabilityQosPolicyKind;
import DDS.ResourceLimitsQosPolicy;
import DDS.SubscriberQos;
import DDS.TimeBasedFilterQosPolicy;
import DDS.TopicDataQosPolicy;
import DDS.TopicQos;
import DDS.TransportPriorityQosPolicy;
import DDS.UserDataQosPolicy;
import DDS.WriterDataLifecycleQosPolicy;

/**
 * @author  Steven Stallion
 */
public class QosPolicies {

    public static DataReaderQos newDataReaderQos() {
        DataReaderQos qos = new DataReaderQos();

        qos.deadline = newDeadlineQosPolicy();
        qos.destination_order = newDestinationOrderQosPolicy();
        qos.durability = newDurabilityQosPolicy();
        qos.history = newHistoryQosPolicy();
        qos.latency_budget = newLatencyBudgetQosPolicy();
        qos.liveliness = newLivelinessQosPolicy();
        qos.ownership = newOwnershipQosPolicy();
        qos.reader_data_lifecycle = newReaderDataLifecycleQosPolicy();
        qos.reliability = newReliabilityQosPolicy();
        qos.resource_limits = newResourceLimitsQosPolicy();
        qos.time_based_filter = newTimeBasedFilterQosPolicy();
        qos.user_data = newUserDataQosPolicy();

        return qos;
    }

    public static DataWriterQos newDataWriterQos() {
        DataWriterQos qos = new DataWriterQos();

        qos.deadline = newDeadlineQosPolicy();
        qos.destination_order = newDestinationOrderQosPolicy();
        qos.durability = newDurabilityQosPolicy();
        qos.durability_service = newDurabilityServiceQosPolicy();
        qos.history = newHistoryQosPolicy();
        qos.latency_budget = newLatencyBudgetQosPolicy();
        qos.lifespan = newLifespanQosPolicy();
        qos.liveliness = newLivelinessQosPolicy();
        qos.ownership = newOwnershipQosPolicy();
        qos.ownership_strength = newOwnershipStrengthQosPolicy();
        qos.reliability = newReliabilityQosPolicy();
        qos.resource_limits = newResourceLimitsQosPolicy();
        qos.transport_priority = newTransportPriorityQosPolicy();
        qos.user_data = newUserDataQosPolicy();
        qos.writer_data_lifecycle = newWriterDataLifecycleQosPolicy();

        return qos;
    }

    public static DeadlineQosPolicy newDeadlineQosPolicy() {
        DeadlineQosPolicy policy = new DeadlineQosPolicy();

        policy.period = new Duration_t();

        return policy;
    }

    public static DestinationOrderQosPolicy newDestinationOrderQosPolicy() {
        DestinationOrderQosPolicy policy = new DestinationOrderQosPolicy();

        policy.kind = DestinationOrderQosPolicyKind.BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;

        return policy;
    }

    public static DurabilityQosPolicy newDurabilityQosPolicy() {
        DurabilityQosPolicy policy = new DurabilityQosPolicy();

        policy.kind = DurabilityQosPolicyKind.VOLATILE_DURABILITY_QOS;

        return policy;
    }

    public static DurabilityServiceQosPolicy newDurabilityServiceQosPolicy() {
        DurabilityServiceQosPolicy policy = new DurabilityServiceQosPolicy();

        policy.history_kind = HistoryQosPolicyKind.KEEP_LAST_HISTORY_QOS;
        policy.service_cleanup_delay = new Duration_t();

        return policy;
    }

    public static EntityFactoryQosPolicy newEntityFactoryQosPolicy() {
        return new EntityFactoryQosPolicy();
    }

    public static GroupDataQosPolicy newGroupDataQosPolicy() {
        GroupDataQosPolicy policy = new GroupDataQosPolicy();

        policy.value = new byte[0];

        return policy;
    }

    public static HistoryQosPolicy newHistoryQosPolicy() {
        HistoryQosPolicy policy = new HistoryQosPolicy();

        policy.kind = HistoryQosPolicyKind.KEEP_LAST_HISTORY_QOS;

        return policy;
    }

    public static LatencyBudgetQosPolicy newLatencyBudgetQosPolicy() {
        LatencyBudgetQosPolicy policy = new LatencyBudgetQosPolicy();

        policy.duration = new Duration_t();

        return policy;
    }

    public static LifespanQosPolicy newLifespanQosPolicy() {
        LifespanQosPolicy policy = new LifespanQosPolicy();

        policy.duration = new Duration_t();

        return policy;
    }

    public static LivelinessQosPolicy newLivelinessQosPolicy() {
        LivelinessQosPolicy policy = new LivelinessQosPolicy();

        policy.kind = LivelinessQosPolicyKind.AUTOMATIC_LIVELINESS_QOS;
        policy.lease_duration = new Duration_t();

        return policy;
    }

    public static OwnershipQosPolicy newOwnershipQosPolicy() {
        OwnershipQosPolicy policy = new OwnershipQosPolicy();

        policy.kind = OwnershipQosPolicyKind.SHARED_OWNERSHIP_QOS;

        return policy;
    }

    public static OwnershipStrengthQosPolicy newOwnershipStrengthQosPolicy() {
        return new OwnershipStrengthQosPolicy();
    }

    public static DomainParticipantQos newParticipantQos() {
        DomainParticipantQos qos = new DomainParticipantQos();

        qos.entity_factory = newEntityFactoryQosPolicy();
        qos.user_data = newUserDataQosPolicy();

        return qos;
    }

    public static PartitionQosPolicy newPartitionQosPolicy() {
        PartitionQosPolicy policy = new PartitionQosPolicy();

        policy.name = new String[0];

        return policy;
    }

    public static PresentationQosPolicy newPresentationQosPolicy() {
        PresentationQosPolicy policy = new PresentationQosPolicy();

        policy.access_scope = PresentationQosPolicyAccessScopeKind.INSTANCE_PRESENTATION_QOS;

        return policy;
    }

    public static PublisherQos newPublisherQos() {
        PublisherQos qos = new PublisherQos();

        qos.entity_factory = newEntityFactoryQosPolicy();
        qos.group_data = newGroupDataQosPolicy();
        qos.partition = newPartitionQosPolicy();
        qos.presentation = newPresentationQosPolicy();

        return qos;
    }

    public static ReaderDataLifecycleQosPolicy newReaderDataLifecycleQosPolicy() {
        ReaderDataLifecycleQosPolicy policy = new ReaderDataLifecycleQosPolicy();

        policy.autopurge_disposed_samples_delay = new Duration_t();
        policy.autopurge_nowriter_samples_delay = new Duration_t();

        return policy;
    }

    public static ReliabilityQosPolicy newReliabilityQosPolicy() {
        ReliabilityQosPolicy policy = new ReliabilityQosPolicy();

        policy.kind = ReliabilityQosPolicyKind.BEST_EFFORT_RELIABILITY_QOS;
        policy.max_blocking_time = new Duration_t();

        return policy;
    }

    public static ResourceLimitsQosPolicy newResourceLimitsQosPolicy() {
        return new ResourceLimitsQosPolicy();
    }

    public static TimeBasedFilterQosPolicy newTimeBasedFilterQosPolicy() {
        TimeBasedFilterQosPolicy policy = new TimeBasedFilterQosPolicy();

        policy.minimum_separation = new Duration_t();

        return policy;
    }

    public static TopicDataQosPolicy newTopicDataQosPolicy() {
        TopicDataQosPolicy policy = new TopicDataQosPolicy();

        policy.value = new byte[0];

        return policy;
    }

    public static SubscriberQos newSubscriberQos() {
        SubscriberQos qos = new SubscriberQos();

        qos.entity_factory = newEntityFactoryQosPolicy();
        qos.group_data = newGroupDataQosPolicy();
        qos.partition = newPartitionQosPolicy();
        qos.presentation = newPresentationQosPolicy();

        return qos;
    }

    public static TopicQos newTopicQos() {
        TopicQos qos = new TopicQos();

        qos.deadline = newDeadlineQosPolicy();
        qos.destination_order = newDestinationOrderQosPolicy();
        qos.durability = newDurabilityQosPolicy();
        qos.durability_service = newDurabilityServiceQosPolicy();
        qos.history = newHistoryQosPolicy();
        qos.latency_budget = newLatencyBudgetQosPolicy();
        qos.lifespan = newLifespanQosPolicy();
        qos.liveliness = newLivelinessQosPolicy();
        qos.ownership = newOwnershipQosPolicy();
        qos.reliability = newReliabilityQosPolicy();
        qos.resource_limits = newResourceLimitsQosPolicy();
        qos.topic_data = newTopicDataQosPolicy();
        qos.transport_priority = newTransportPriorityQosPolicy();

        return qos;
    }

    public static TransportPriorityQosPolicy newTransportPriorityQosPolicy() {
        return new TransportPriorityQosPolicy();
    }

    public static UserDataQosPolicy newUserDataQosPolicy() {
        UserDataQosPolicy policy = new UserDataQosPolicy();

        policy.value = new byte[0];

        return policy;
    }

    public static WriterDataLifecycleQosPolicy newWriterDataLifecycleQosPolicy() {
        return new WriterDataLifecycleQosPolicy();
    }

    //

    private QosPolicies() {}
}
