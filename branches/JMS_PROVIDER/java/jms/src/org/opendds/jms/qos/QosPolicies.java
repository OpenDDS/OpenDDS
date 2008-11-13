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

    public static DeadlineQosPolicy defaultDeadlineQosPolicy() {
        DeadlineQosPolicy policy = new DeadlineQosPolicy();

        policy.period = new Duration_t();

        return policy;
    }

    public static DestinationOrderQosPolicy defaultDestinationOrderQosPolicy() {
        DestinationOrderQosPolicy policy = new DestinationOrderQosPolicy();

        policy.kind = DestinationOrderQosPolicyKind.BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;

        return policy;
    }

    public static DurabilityQosPolicy defaultDurabilityQosPolicy() {
        DurabilityQosPolicy policy = new DurabilityQosPolicy();

        policy.kind = DurabilityQosPolicyKind.VOLATILE_DURABILITY_QOS;
        policy.service_cleanup_delay = new Duration_t();

        return policy;
    }

    public static DurabilityServiceQosPolicy defaultDurabilityServiceQosPolicy() {
        DurabilityServiceQosPolicy policy = new DurabilityServiceQosPolicy();

        policy.history_kind = HistoryQosPolicyKind.KEEP_LAST_HISTORY_QOS;
        policy.service_cleanup_delay = new Duration_t();

        return policy;
    }

    public static HistoryQosPolicy defaultHistoryQosPolicy() {
        HistoryQosPolicy policy = new HistoryQosPolicy();

        policy.kind = HistoryQosPolicyKind.KEEP_LAST_HISTORY_QOS;

        return policy;
    }

    public static LatencyBudgetQosPolicy defaultLatencyBudgetQosPolicy() {
        LatencyBudgetQosPolicy policy = new LatencyBudgetQosPolicy();

        policy.duration = new Duration_t();

        return policy;
    }

    public static LifespanQosPolicy defaultLifespanQosPolicy() {
        LifespanQosPolicy policy = new LifespanQosPolicy();

        policy.duration = new Duration_t();

        return policy;
    }

    public static LivelinessQosPolicy defaultLivelinessQosPolicy() {
        LivelinessQosPolicy policy = new LivelinessQosPolicy();

        policy.kind = LivelinessQosPolicyKind.AUTOMATIC_LIVELINESS_QOS;
        policy.lease_duration = new Duration_t();

        return policy;
    }

    public static OwnershipQosPolicy defaultOwnershipQosPolicy() {
        OwnershipQosPolicy policy = new OwnershipQosPolicy();

        policy.kind = OwnershipQosPolicyKind.SHARED_OWNERSHIP_QOS;

        return policy;
    }

    public static ReliabilityQosPolicy defaultReliabilityQosPolicy() {
        ReliabilityQosPolicy policy = new ReliabilityQosPolicy();

        policy.kind = ReliabilityQosPolicyKind.BEST_EFFORT_RELIABILITY_QOS;
        policy.max_blocking_time = new Duration_t();

        return policy;
    }

    public static ResourceLimitsQosPolicy defaultResourceLimitsQosPolicy() {
        return new ResourceLimitsQosPolicy();
    }

    public static TopicDataQosPolicy defaultTopicDataQosPolicy() {
        TopicDataQosPolicy policy = new TopicDataQosPolicy();

        policy.value = new byte[0];

        return policy;
    }

    public static TopicQos defaultTopicQos() {
        TopicQos policy = new TopicQos();

        policy.deadline = defaultDeadlineQosPolicy();
        policy.destination_order = defaultDestinationOrderQosPolicy();
        policy.durability = defaultDurabilityQosPolicy();
        policy.durability_service = defaultDurabilityServiceQosPolicy();
        policy.history = defaultHistoryQosPolicy();
        policy.latency_budget = defaultLatencyBudgetQosPolicy();
        policy.lifespan = defaultLifespanQosPolicy();
        policy.liveliness = defaultLivelinessQosPolicy();
        policy.ownership = defaultOwnershipQosPolicy();
        policy.reliability = defaultReliabilityQosPolicy();
        policy.resource_limits = defaultResourceLimitsQosPolicy();
        policy.topic_data = defaultTopicDataQosPolicy();
        policy.transport_priority = defaultTransportPriorityQosPolicy();

        return policy;
    }

    public static TransportPriorityQosPolicy defaultTransportPriorityQosPolicy() {
        return new TransportPriorityQosPolicy();
    }

    //

    private QosPolicies() {}
}
