/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.qos;

import java.util.Properties;

import DDS.DataWriterQos;
import DDS.DestinationOrderQosPolicyKind;
import DDS.HistoryQosPolicyKind;
import DDS.LivelinessQosPolicyKind;
import DDS.ReliabilityQosPolicyKind;

import org.opendds.jms.common.util.Logger;
import org.opendds.jms.common.util.PropertiesHelper;

/**
 * @author  Steven Stallion
 */
public class DataWriterQosPolicy implements QosPolicy<DataWriterQos> {
    private static Logger logger = Logger.getLogger(DataWriterQosPolicy.class);

    private Properties properties;

    public DataWriterQosPolicy() {
        this(new Properties());
    }

    public DataWriterQosPolicy(Properties properties) {
        this.properties = properties;
    }

    public DataWriterQosPolicy(String value) {
        this(PropertiesHelper.valueOf(value));
    }

    public void setQos(DataWriterQos qos) {
        assert qos != null;

        PropertiesHelper.Property property;
        PropertiesHelper helper = new PropertiesHelper(properties);

        // USER_DATA QosPolicy
        property = helper.find("USER_DATA.value");
        if (property.exists()) {
            qos.user_data.value = property.asBytes();
        }

        // DURABILITY QosPolicy (reserved)
        property = helper.find("DURABILITY.kind");
        if (property.exists()) {
            logger.warn("DURABILITY QosPolicy is reserved for internal use!");
        }

        // DURABILITY_SERVICE QosPolicy
        property = helper.find("DURABILITY_SERVICE.service_cleanup_delay.sec");
        if (property.exists()) {
            qos.durability_service.service_cleanup_delay.sec = property.asInt();
        }

        property = helper.find("DURABILITY_SERVICE.service_cleanup_delay.nanosec");
        if (property.exists()) {
            qos.durability_service.service_cleanup_delay.nanosec = property.asInt();
        }

        property = helper.find("DURABILITY_SERVICE.history_kind");
        if (property.exists()) {
            if (property.equals("KEEP_LAST")) {
                qos.durability_service.history_kind = HistoryQosPolicyKind.KEEP_LAST_HISTORY_QOS;

            } else if (property.equals("KEEP_ALL")) {
                qos.durability_service.history_kind = HistoryQosPolicyKind.KEEP_ALL_HISTORY_QOS;
            }
        }

        property = helper.find("DURABILITY_SERVICE.history_depth");
        if (property.exists()) {
            qos.durability_service.history_depth = property.asInt();
        }

        property = helper.find("DURABILITY_SERVICE.max_samples");
        if (property.exists()) {
            qos.durability_service.max_samples = property.asInt();
        }

        property = helper.find("DURABILITY_SERVICE.max_instances");
        if (property.exists()) {
            qos.durability_service.max_instances = property.asInt();
        }

        property = helper.find("DURABILITY_SERVICE.max_samples_per_instance");
        if (property.exists()) {
            qos.durability_service.max_samples_per_instance = property.asInt();
        }

        // DEADLINE QosPolicy
        property = helper.find("DEADLINE.period.sec");
        if (property.exists()) {
            qos.deadline.period.sec = property.asInt();
        }

        property = helper.find("DEADLINE.period.nanosec");
        if (property.exists()) {
            qos.deadline.period.nanosec = property.asInt();
        }

        // LATENCY_BUDGET QosPolicy
        property = helper.find("LATENCY_BUDGET.duration.sec");
        if (property.exists()) {
            qos.latency_budget.duration.sec = property.asInt();
        }

        property = helper.find("LATENCY_BUDGET.duration.nanosec");
        if (property.exists()) {
            qos.latency_budget.duration.nanosec = property.asInt();
        }

        // OWNERSHIP_STRENGTH QosPolicy
        property = helper.find("OWNERSHIP_STRENGTH.value");
        if (property.exists()) {
            qos.ownership_strength.value = property.asInt();
        }

        // LIVELINESS QosPolicy
        property = helper.find("LIVELINESS.kind");
        if (property.exists()) {
            if (property.equals("AUTOMATIC")) {
                qos.liveliness.kind = LivelinessQosPolicyKind.AUTOMATIC_LIVELINESS_QOS;

            } else if (property.equals("MANUAL_BY_PARTICIPANT")) {
                qos.liveliness.kind = LivelinessQosPolicyKind.MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;

            } else if (property.equals("MANUAL_BY_TOPIC")) {
                qos.liveliness.kind = LivelinessQosPolicyKind.MANUAL_BY_TOPIC_LIVELINESS_QOS;
            }
        }

        property = helper.find("LIVELINESS.lease_duration.sec");
        if (property.exists()) {
            qos.liveliness.lease_duration.sec = property.asInt();
        }

        property = helper.find("LIVELINESS.lease_duration.nanosec");
        if (property.exists()) {
            qos.liveliness.lease_duration.nanosec = property.asInt();
        }

        // RELIABILITY QosPolicy
        property = helper.find("RELIABILITY.kind");
        if (property.exists()) {
            if (property.equals("BEST_EFFORT")) {
                qos.reliability.kind = ReliabilityQosPolicyKind.BEST_EFFORT_RELIABILITY_QOS;

            } else if (property.equals("RELIABLE")) {
                qos.reliability.kind = ReliabilityQosPolicyKind.RELIABLE_RELIABILITY_QOS;
            }
        }

        property = helper.find("RELIABILITY.max_blocking_time.sec");
        if (property.exists()) {
            qos.reliability.max_blocking_time.sec = property.asInt();
        }

        property = helper.find("RELIABILITY.max_blocking_time.nanosec");
        if (property.exists()) {
            qos.reliability.max_blocking_time.nanosec = property.asInt();
        }

        // TRANSPORT_PRIORITY QosPolicy
        property = helper.find("TRANSPORT_PRIORITY.value");
        if (property.exists()) {
            qos.transport_priority.value = property.asInt();
        }

        // LIFESPAN QosPolicy (reserved)
        property = helper.find("LIFESPAN.duration.sec");
        if (property.exists()) {
            logger.warn("LIFESPAN QosPolicy is reserved for internal use!");
        }

        property = helper.find("LIFESPAN.duration.nanosec");
        if (property.exists()) {
            logger.warn("LIFESPAN QosPolicy is reserved for internal use!");
        }

        // DESTINATION_ORDER QosPolicy
        property = helper.find("DESTINATION_ORDER.kind");
        if (property.exists()) {
            if (property.equals("BY_RECEPTION_TIMESTAMP")) {
                qos.destination_order.kind =
                    DestinationOrderQosPolicyKind.BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;

            } else if (property.equals("BY_SOURCE_TIMESTAMP")) {
                qos.destination_order.kind =
                    DestinationOrderQosPolicyKind.BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
            }
        }

        // HISTORY QosPolicy
        property = helper.find("HISTORY.kind");
        if (property.exists()) {
            if (property.equals("KEEP_LAST")) {
                qos.history.kind = HistoryQosPolicyKind.KEEP_LAST_HISTORY_QOS;

            } else if (property.equals("KEEP_ALL")) {
                qos.history.kind = HistoryQosPolicyKind.KEEP_ALL_HISTORY_QOS;
            }
        }

        property = helper.find("HISTORY.depth");
        if (property.exists()) {
            qos.history.depth = property.asInt();
        }

        // RESOURCE_LIMITS QosPolicy
        property = helper.find("RESOURCE_LIMITS.max_samples");
        if (property.exists()) {
            qos.resource_limits.max_samples = property.asInt();
        }

        property = helper.find("RESOURCE_LIMITS.max_instances");
        if (property.exists()) {
            qos.resource_limits.max_instances = property.asInt();
        }

        property = helper.find("RESOURCE_LIMITS.max_samples_per_instance");
        if (property.exists()) {
            qos.resource_limits.max_samples_per_instance = property.asInt();
        }

        // WRITER_DATA_LIFECYCLE QosPolicy
        property = helper.find("WRITER_DATA_LIFECYCLE.autodispose_unregistered_instances");
        if (property.exists()) {
            qos.writer_data_lifecycle.autodispose_unregistered_instances = property.asBoolean();
        }
    }

    @Override
    public String toString() {
        return properties.toString();
    }
}
