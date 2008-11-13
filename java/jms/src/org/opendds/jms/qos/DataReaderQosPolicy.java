/*
 * $Id$
 */

package org.opendds.jms.qos;

import java.util.Properties;

import DDS.DataReaderQos;
import DDS.DataReaderQosHolder;
import DDS.DurabilityQosPolicyKind;
import DDS.LivelinessQosPolicyKind;

import org.opendds.jms.util.PropertiesHelper;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class DataReaderQosPolicy implements QosPolicy<DataReaderQos> {
    private PropertiesHelper helper;

    public DataReaderQosPolicy(String value) {
        this(PropertiesHelper.valueOf(value));
    }

    public DataReaderQosPolicy(Properties properties) {
        helper = new PropertiesHelper(properties);
    }

    public void setQos(DataReaderQosHolder holder) {
        setQos(holder.value);
    }

    public void setQos(DataReaderQos qos) {
        PropertiesHelper.Property property;

        // USER_DATA
        property = helper.find("USER_DATA.value");
        if (property.exists()) {
            qos.user_data.value = property.asBytes();
        }

        // DURABILITY
        property = helper.find("DURABILITY.kind");
        if (property.exists()) {

            if (property.equals("VOLATILE")) {
                qos.durability.kind = DurabilityQosPolicyKind.VOLATILE_DURABILITY_QOS;

            } else if (property.equals("TRANSIENT_LOCAL")) {
                qos.durability.kind = DurabilityQosPolicyKind.TRANSIENT_LOCAL_DURABILITY_QOS;

            } else if (property.equals("TRANSIENT")) {
                qos.durability.kind = DurabilityQosPolicyKind.TRANSIENT_DURABILITY_QOS;

            } else if (property.equals("PERSISTENT")) {
                qos.durability.kind = DurabilityQosPolicyKind.PERSISTENT_DURABILITY_QOS;
            }
        }

        // DEADLINE
        property = helper.find("DEADLINE.period.sec");
        if (property.exists()) {
            qos.deadline.period.sec = property.asInt();
        }

        property = helper.find("DEADLINE.period.nanosec");
        if (property.exists()) {
            qos.deadline.period.nanosec = property.asInt();
        }

        // LATENCY_BUDGET
        property = helper.find("LATENCY_BUDGET.duration.sec");
        if (property.exists()) {
            qos.latency_budget.duration.sec = property.asInt();
        }

        property = helper.find("LATENCY_BUDGET.duration.nanosec");
        if (property.exists()) {
            qos.latency_budget.duration.nanosec = property.asInt();
        }

        // LIVELINESS
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
    }
}
