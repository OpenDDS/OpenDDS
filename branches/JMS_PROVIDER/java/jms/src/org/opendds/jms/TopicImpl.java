/*
 * $Id$
 */

package org.opendds.jms;

import java.io.Serializable;

import javax.jms.JMSException;
import javax.jms.Topic;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import DDS.DomainParticipant;
import DDS.TopicQosHolder;

import org.opendds.jms.qos.DataReaderQosPolicy;
import org.opendds.jms.qos.DataWriterQosPolicy;
import org.opendds.jms.qos.QosPolicies;
import org.opendds.jms.qos.TopicQosPolicy;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TopicImpl implements Serializable, Topic {
    private static Log log = LogFactory.getLog(TopicImpl.class);

    private String topicName;
    private DataReaderQosPolicy dataReaderQosPolicy;
    private DataWriterQosPolicy dataWriterQosPolicy;
    private TopicQosPolicy topicQosPolicy;

    public TopicImpl(String topicName) {
        this.topicName = topicName;
    }

    public TopicImpl(String topicName,
                     DataReaderQosPolicy dataReaderQosPolicy,
                     DataWriterQosPolicy dataWriterQosPolicy,
                     TopicQosPolicy topicQosPolicy) {

        this(topicName);
        this.dataReaderQosPolicy = dataReaderQosPolicy;
        this.dataWriterQosPolicy = dataWriterQosPolicy;
        this.topicQosPolicy = topicQosPolicy;
    }

    public String getTopicName() {
        return topicName;
    }

    public DataReaderQosPolicy getDataReaderQosPolicy() {
        return dataReaderQosPolicy;
    }

    public DataWriterQosPolicy getDataWriterQosPolicy() {
        return dataWriterQosPolicy;
    }

    public TopicQosPolicy getTopicQosPolicy() {
        return topicQosPolicy;
    }

    public DDS.Topic createDDSTopic(ConnectionImpl connection) throws JMSException {
        TopicQosHolder holder = new TopicQosHolder(QosPolicies.newTopicQos());

        DomainParticipant participant = connection.getParticipant();
        participant.get_default_topic_qos(holder);

        if (topicQosPolicy != null) {
            topicQosPolicy.setQos(holder.value);
        }

        DDS.Topic topic = participant.create_topic(topicName, connection.getTypeName(), holder.value, null);
        if (topic == null) {
            throw new JMSException("Unable to create Topic; please check logs");
        }
        if (log.isDebugEnabled()) {
            log.debug(String.format("Created %s using %s", topic, holder.value));
        }

        return topic;
    }

    @Override
    public String toString() {
        return getTopicName();
    }
}
