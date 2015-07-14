/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.qos;

import java.util.Properties;

import DDS.TopicQos;

import org.opendds.jms.common.util.PropertiesHelper;

/**
 * @author  Steven Stallion
 */
public class TopicQosPolicy implements QosPolicy<TopicQos> {
    private Properties properties;

    public TopicQosPolicy() {
        this(new Properties());
    }

    public TopicQosPolicy(Properties properties) {
        this.properties = properties;
    }

    public TopicQosPolicy(String value) {
        this(PropertiesHelper.valueOf(value));
    }

    public void setQos(TopicQos qos) {
        assert qos != null;

        PropertiesHelper.Property property;
        PropertiesHelper helper = new PropertiesHelper(properties);

        // TopicQos QosPolicies which are shared with DataReaderQos
        // and DataWriterQos are considered to be informational only.
        // *_QOS_USE_TOPIC_QOS is not used by the JMS provider,
        // therefore we are only interested in TopicDataQosPolicy.

        // TOPIC_DATA QosPolicy
        property = helper.find("TOPIC_DATA.value");
        if (property.exists()) {
            qos.topic_data.value = property.asBytes();
        }
    }

    @Override
    public String toString() {
        return properties.toString();
    }
}
