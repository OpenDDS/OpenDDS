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

    public static DeadlineQosPolicy createDeadlineQosPolicy() {
        DeadlineQosPolicy policy = new DeadlineQosPolicy();

        policy.period = new Duration_t();

        return policy;
    }

    public static DestinationOrderQosPolicy createDestinationOrderQosPolicy() {
        DestinationOrderQosPolicy policy = new DestinationOrderQosPolicy();

        policy.kind = DestinationOrderQosPolicyKind.BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;

        return policy;
    }

    public static DurabilityQosPolicy createDurabilityQosPolicy() {
        DurabilityQosPolicy policy = new DurabilityQosPolicy();

        policy.kind = DurabilityQosPolicyKind.VOLATILE_DURABILITY_QOS;
        policy.service_cleanup_delay = new Duration_t();

        return policy;
    }

    public static DurabilityServiceQosPolicy createDurabilityServiceQosPolicy() {
        DurabilityServiceQosPolicy policy = new DurabilityServiceQosPolicy();

        policy.history_kind = HistoryQosPolicyKind.KEEP_LAST_HISTORY_QOS;
        policy.service_cleanup_delay = new Duration_t();

        return policy;
    }

    public static HistoryQosPolicy createHistoryQosPolicy() {
        HistoryQosPolicy policy = new HistoryQosPolicy();

        policy.kind = HistoryQosPolicyKind.KEEP_LAST_HISTORY_QOS;

        return policy;
    }

    public static LatencyBudgetQosPolicy createLatencyBudgetQosPolicy() {
        LatencyBudgetQosPolicy policy = new LatencyBudgetQosPolicy();

        policy.duration = new Duration_t();

        return policy;
    }

    public static LifespanQosPolicy createLifespanQosPolicy() {
        LifespanQosPolicy policy = new LifespanQosPolicy();

        policy.duration = new Duration_t();

        return policy;
    }

    public static LivelinessQosPolicy createLivelinessQosPolicy() {
        LivelinessQosPolicy policy = new LivelinessQosPolicy();

        policy.kind = LivelinessQosPolicyKind.AUTOMATIC_LIVELINESS_QOS;
        policy.lease_duration = new Duration_t();

        return policy;
    }

    public static OwnershipQosPolicy createOwnershipQosPolicy() {
        OwnershipQosPolicy policy = new OwnershipQosPolicy();

        policy.kind = OwnershipQosPolicyKind.SHARED_OWNERSHIP_QOS;

        return policy;
    }

    public static ReliabilityQosPolicy createReliabilityQosPolicy() {
        ReliabilityQosPolicy policy = new ReliabilityQosPolicy();

        policy.kind = ReliabilityQosPolicyKind.BEST_EFFORT_RELIABILITY_QOS;
        policy.max_blocking_time = new Duration_t();

        return policy;
    }

    public static ResourceLimitsQosPolicy createResourceLimitsQosPolicy() {
        return new ResourceLimitsQosPolicy();
    }

    public static TopicDataQosPolicy createTopicDataQosPolicy() {
        TopicDataQosPolicy policy = new TopicDataQosPolicy();

        policy.value = new byte[0];

        return policy;
    }

    public static TopicQos createTopicQos() {
        TopicQos policy = new TopicQos();

        policy.deadline = createDeadlineQosPolicy();
        policy.destination_order = createDestinationOrderQosPolicy();
        policy.durability = createDurabilityQosPolicy();
        policy.durability_service = createDurabilityServiceQosPolicy();
        policy.history = createHistoryQosPolicy();
        policy.latency_budget = createLatencyBudgetQosPolicy();
        policy.lifespan = createLifespanQosPolicy();
        policy.liveliness = createLivelinessQosPolicy();
        policy.ownership = createOwnershipQosPolicy();
        policy.reliability = createReliabilityQosPolicy();
        policy.resource_limits = createResourceLimitsQosPolicy();
        policy.topic_data = createTopicDataQosPolicy();
        policy.transport_priority = createTransportPriorityQosPolicy();

        return policy;
    }

    public static TransportPriorityQosPolicy createTransportPriorityQosPolicy() {
        return new TransportPriorityQosPolicy();
    }

    //

    private QosPolicies() {}
}
