/*
 * $Id$
 */

package org.opendds.jms.management;

import java.io.Serializable;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.opendds.jms.TopicImpl;
import org.opendds.jms.management.annotation.Attribute;
import org.opendds.jms.management.annotation.Description;
import org.opendds.jms.management.annotation.KeyProperty;
import org.opendds.jms.management.annotation.Operation;
import org.opendds.jms.util.JndiHelper;
import org.opendds.jms.util.PropertiesHelper;
import org.opendds.jms.util.Strings;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
@Description("OpenDDS Destination MBean")
public class Destination extends DynamicMBeanSupport implements Serializable, ServiceMBean {
    private Log log;

    private boolean active;
    private String destination;
    private String type;
    private String jndiName;
    private String dataReaderQosPolicy;
    private String dataWriterQosPolicy;
    private String topicQosPolicy;

    private JndiHelper helper = new JndiHelper();

    @Attribute(readOnly = true)
    public String getDestination() {
        return destination;
    }

    @KeyProperty
    public void setDestination(String destination) {
        this.destination = destination;
    }

    @Attribute(readOnly = true)
    public String getType() {
        return type;
    }

    @KeyProperty
    public void setType(String type) {
        // Currently, only 'Topic' destination types are supported.
        // This is a placeholder until Queuing support is added.
        if (!"Topic".equals(type)) {
            throw new IllegalArgumentException(type);
        }
        this.type = type;
    }

    @Attribute(required = true)
    public String getJndiName() {
        return jndiName;
    }

    public void setJndiName(String jndiName) {
        if (Strings.isEmpty(jndiName)) {
            throw new IllegalArgumentException();
        }
        this.jndiName = jndiName;
    }

    @Attribute
    public String getDataReaderQosPolicy() {
        return dataReaderQosPolicy;
    }

    public void setDataReaderQosPolicy(String dataReaderQosPolicy) {
        this.dataReaderQosPolicy = dataReaderQosPolicy;
    }

    @Attribute
    public String getDataWriterQosPolicy() {
        return dataWriterQosPolicy;
    }

    public void setDataWriterQosPolicy(String dataWriterQosPolicy) {
        this.dataWriterQosPolicy = dataWriterQosPolicy;
    }

    @Attribute
    public String getTopicQosPolicy() {
        return topicQosPolicy;
    }

    public void setTopicQosPolicy(String topicQosPolicy) {
        this.topicQosPolicy = topicQosPolicy;
    }

    @Attribute
    public boolean isActive() {
        return active;
    }

    @Operation
    public void start() throws Exception {
        if (isActive()) {
            throw new IllegalStateException(getDestination() + " already started!");
        }

        verify();

        log = LogFactory.getLog(getDestination());
        if (log.isInfoEnabled()) {
            log.info("Binding to JNDI name: " + jndiName);
        }

        TopicImpl topic = new TopicImpl(destination);

        topic.setDataReaderQosPolicy(
            PropertiesHelper.forValue(dataReaderQosPolicy));

        topic.setDataWriterQosPolicy(
            PropertiesHelper.forValue(dataWriterQosPolicy));

        topic.setTopicQosPolicy(
            PropertiesHelper.forValue(topicQosPolicy));

        helper.bind(jndiName, topic);

        active = true;
    }

    @Operation
    public void stop() throws Exception {
        if (!isActive()) {
            throw new IllegalStateException(getDestination() + " already stopped!");
        }

        if (log.isInfoEnabled()) {
            log.info("Unbinding JNDI name: " + jndiName);
        }

        helper.unbind(jndiName);
        log = null;

        active = false;
    }
}
