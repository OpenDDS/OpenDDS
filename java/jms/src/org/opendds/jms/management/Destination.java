/*
 * $Id$
 */

package org.opendds.jms.management;

import java.io.Serializable;
import java.util.Properties;

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
    private String jndiName;
    private String type;

    private Properties dataReaderQosPolicy;
    private Properties dataWriterQosPolicy;
    private Properties topicQosPolicy; 

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
        String value = null;
        if (dataReaderQosPolicy != null) {
            value = PropertiesHelper.valueOf(dataReaderQosPolicy);
        }
        return value;
    }

    public void setDataReaderQosPolicy(String value) {
        dataReaderQosPolicy = PropertiesHelper.forValue(value);
    }

    @Attribute
    public String getDataWriterQosPolicy() {
        String value = null;
        if (dataWriterQosPolicy != null) {
            value = PropertiesHelper.valueOf(dataWriterQosPolicy);
        }
        return value;
    }

    public void setDataWriterQosPolicy(String value) {
        dataWriterQosPolicy = PropertiesHelper.forValue(value);
    }

    @Attribute
    public String getTopicQosPolicy() {
        String value = null;
        if (topicQosPolicy != null) {
            value = PropertiesHelper.valueOf(topicQosPolicy);
        }
        return value;
    }

    public void setTopicQosPolicy(String value) {
        topicQosPolicy = PropertiesHelper.forValue(value);
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

        topic.setDataReaderQosPolicy(dataReaderQosPolicy);
        topic.setDataWriterQosPolicy(dataWriterQosPolicy);
        topic.setTopicQosPolicy(topicQosPolicy);
        
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
