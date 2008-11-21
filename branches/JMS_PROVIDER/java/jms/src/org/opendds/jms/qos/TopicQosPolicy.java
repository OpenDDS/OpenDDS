/*
 * $Id$
 */

package org.opendds.jms.qos;

import java.util.Properties;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import DDS.TopicQos;

import org.opendds.jms.common.util.PropertiesHelper;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TopicQosPolicy implements QosPolicy<TopicQos> {
    private static Log log = LogFactory.getLog(TopicQosPolicy.class);

    private Properties properties;

    public TopicQosPolicy(String value) {
        this(PropertiesHelper.valueOf(value));
    }

    public TopicQosPolicy(Properties properties) {
        this.properties = properties;
    }

    public void setQos(TopicQos qos) {
        if (log.isDebugEnabled()) {
            log.debug(String.format("Configuring %s %s", qos, PropertiesHelper.valueOf(properties)));
        }

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
        return PropertiesHelper.valueOf(properties);
    }
}
