/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.qos;

import java.util.Properties;

import DDS.DataReaderQos;
import DDS.DestinationOrderQosPolicyKind;
import DDS.HistoryQosPolicyKind;
import DDS.LivelinessQosPolicyKind;
import DDS.ReliabilityQosPolicyKind;

import org.opendds.jms.common.util.Logger;
import org.opendds.jms.common.util.PropertiesHelper;

/**
 * @author  Steven Stallion
 */
public class DataReaderQosPolicy implements QosPolicy<DataReaderQos> {
    private static Logger logger = Logger.getLogger(DataReaderQosPolicy.class);

    private Properties properties;

    public DataReaderQosPolicy() {
        this(new Properties());
    }

    public DataReaderQosPolicy(Properties properties) {
        this.properties = properties;
    }

    public DataReaderQosPolicy(String value) {
        this(PropertiesHelper.valueOf(value));
    }

    public void setQos(DataReaderQos qos) {
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

        // TIME_BASED_FILTER QosPolicy
        property = helper.find("TIME_BASED_FILTER.minimum_separation.sec");
        if (property.exists()) {
            qos.time_based_filter.minimum_separation.sec = property.asInt();
        }

        property = helper.find("TIME_BASED_FILTER.minimum_separation.nanosec");
        if (property.exists()) {
            qos.time_based_filter.minimum_separation.nanosec = property.asInt();
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

        // READER_DATA_LIFECYCLE QosPolicy
        property = helper.find("READER_DATA_LIFECYCLE.autopurge_nowriter_samples_delay.sec");
        if (property.exists()) {
            qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec = property.asInt();
        }

        property = helper.find("READER_DATA_LIFECYCLE.autopurge_nowriter_samples_delay_nanosec");
        if (property.exists()) {
            qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec = property.asInt();
        }
    }

    @Override
    public String toString() {
        return properties.toString();
    }
}
