/*
 * $Id$
 */

package org.opendds.jms.qos;

import DDS.DeadlineQosPolicy;
import DDS.DestinationOrderQosPolicy;
import DDS.DestinationOrderQosPolicyKind;
import DDS.DurabilityQosPolicy;
import DDS.DurabilityQosPolicyKind;
import DDS.DurabilityServiceQosPolicy;
import DDS.Duration_t;
import DDS.HistoryQosPolicy;
import DDS.HistoryQosPolicyKind;
import DDS.LatencyBudgetQosPolicy;
import DDS.LifespanQosPolicy;
import DDS.LivelinessQosPolicy;
import DDS.LivelinessQosPolicyKind;
import DDS.OwnershipQosPolicy;
import DDS.OwnershipQosPolicyKind;
import DDS.ReliabilityQosPolicy;
import DDS.ReliabilityQosPolicyKind;
import DDS.ResourceLimitsQosPolicy;
import DDS.TopicDataQosPolicy;
import DDS.TopicQos;
import DDS.TransportPriorityQosPolicy;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class QosPolicies {

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
        policy.service_cleanup_delay = new Duration_t();

        return policy;
    }

    public static DurabilityServiceQosPolicy newDurabilityServiceQosPolicy() {
        DurabilityServiceQosPolicy policy = new DurabilityServiceQosPolicy();

        policy.history_kind = HistoryQosPolicyKind.KEEP_LAST_HISTORY_QOS;
        policy.service_cleanup_delay = new Duration_t();

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

    public static ReliabilityQosPolicy newReliabilityQosPolicy() {
        ReliabilityQosPolicy policy = new ReliabilityQosPolicy();

        policy.kind = ReliabilityQosPolicyKind.BEST_EFFORT_RELIABILITY_QOS;
        policy.max_blocking_time = new Duration_t();

        return policy;
    }

    public static ResourceLimitsQosPolicy newResourceLimitsQosPolicy() {
        return new ResourceLimitsQosPolicy();
    }

    public static TopicDataQosPolicy newTopicDataQosPolicy() {
        TopicDataQosPolicy policy = new TopicDataQosPolicy();

        policy.value = new byte[0];

        return policy;
    }

    public static TopicQos newTopicQos() {
        TopicQos policy = new TopicQos();

        policy.deadline = newDeadlineQosPolicy();
        policy.destination_order = newDestinationOrderQosPolicy();
        policy.durability = newDurabilityQosPolicy();
        policy.durability_service = newDurabilityServiceQosPolicy();
        policy.history = newHistoryQosPolicy();
        policy.latency_budget = newLatencyBudgetQosPolicy();
        policy.lifespan = newLifespanQosPolicy();
        policy.liveliness = newLivelinessQosPolicy();
        policy.ownership = newOwnershipQosPolicy();
        policy.reliability = newReliabilityQosPolicy();
        policy.resource_limits = newResourceLimitsQosPolicy();
        policy.topic_data = newTopicDataQosPolicy();
        policy.transport_priority = newTransportPriorityQosPolicy();

        return policy;
    }

    public static TransportPriorityQosPolicy newTransportPriorityQosPolicy() {
        return new TransportPriorityQosPolicy();
    }

    //

    private QosPolicies() {}
}
