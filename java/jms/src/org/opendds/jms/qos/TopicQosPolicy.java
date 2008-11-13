/*
 * $Id$
 */

package org.opendds.jms.qos;

import java.util.Properties;

import DDS.TopicQos;
import DDS.TopicQosHolder;

import org.opendds.jms.util.PropertiesHelper;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TopicQosPolicy implements QosPolicy<TopicQos> {
    private PropertiesHelper helper;

    public TopicQosPolicy(String value) {
        this(PropertiesHelper.valueOf(value));
    }

    public TopicQosPolicy(Properties properties) {
        helper = new PropertiesHelper(properties);
    }

    public void setQos(TopicQosHolder holder) {
        setQos(holder.value);
    }

    public void setQos(TopicQos qos) {
        PropertiesHelper.Property property;

        // TopicQos QosPolicies which are shared with DataReaderQos
        // and DataWriterQos are considered to be informational only.
        // *_QOS_USE_TOPIC_QOS is not used by the JMS provider,
        // therefore we are only interested in TopicDataQosPolicy.

        // TOPIC_DATA
        property = helper.find("TOPIC_DATA.value");
        if (property.exists()) {
            qos.topic_data.value = property.asBytes();
        }
    }
}
