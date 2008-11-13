/*
 * $Id$
 */

package org.opendds.jms;

import java.io.Serializable;

import javax.jms.JMSException;
import javax.jms.Topic;

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
    private String topicName;

    private DataReaderQosPolicy dataReaderQosPolicy;
    private DataWriterQosPolicy dataWriterQosPolicy;
    private TopicQosPolicy topicQosPolicy;

    public TopicImpl(String topicName) {
        this(topicName, null, null, null);
    }

    public TopicImpl(String topicName,
                     DataReaderQosPolicy dataReaderQosPolicy,
                     DataWriterQosPolicy dataWriterQosPolicy,
                     TopicQosPolicy topicQosPolicy) {

        this.topicName = topicName;
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

    public DDS.Topic createTopic(DomainParticipant participant) throws JMSException {
        TopicQosHolder holder = new TopicQosHolder(QosPolicies.newTopicQos());

        participant.get_default_topic_qos(holder);

        if (topicQosPolicy != null) {
            topicQosPolicy.setQos(holder.value);
        }

        DDS.Topic topic = participant.create_topic(topicName,
            "OpenDDS::JMS::MessagePayload", holder.value, null);

        if (topic == null) {
            throw new JMSException("Unable to create Topic; please check logs");
        }

        return topic;
    }

    @Override
    public String toString() {
        return getTopicName();
    }
}
