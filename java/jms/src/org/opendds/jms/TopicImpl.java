/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.io.Serializable;

import javax.jms.JMSException;
import javax.jms.Topic;

import DDS.DomainParticipant;
import DDS.TopicQosHolder;
import OpenDDS.DCPS.DEFAULT_STATUS_MASK;

import org.opendds.jms.common.util.Logger;
import org.opendds.jms.qos.DataReaderQosPolicy;
import org.opendds.jms.qos.DataWriterQosPolicy;
import org.opendds.jms.qos.QosPolicies;
import org.opendds.jms.qos.TopicQosPolicy;

/**
 * @author  Steven Stallion
 */
public class TopicImpl implements Serializable, Topic {
    private String topicName;
    private DataReaderQosPolicy dataReaderQosPolicy;
    private DataWriterQosPolicy dataWriterQosPolicy;
    private TopicQosPolicy topicQosPolicy;

    public TopicImpl(String topicName) {
        this(topicName,
             new DataReaderQosPolicy(),
             new DataWriterQosPolicy(),
             new TopicQosPolicy());
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

    public DDS.Topic createDDSTopic(ConnectionImpl connection) throws JMSException {
        assert connection != null;

        Logger logger = connection.getLogger();

        TopicQosHolder holder = new TopicQosHolder(QosPolicies.newTopicQos());

        DomainParticipant participant = connection.getParticipant();
        participant.get_default_topic_qos(holder);

        topicQosPolicy.setQos(holder.value);

        DDS.Topic topic = participant.create_topic(topicName,
                                                   connection.getTypeName(),
                                                   holder.value,
                                                   null,
                                                   DEFAULT_STATUS_MASK.value);
        if (topic == null) {
            throw new JMSException("Unable to create Topic; please check logs");
        }
        logger.debug("Created %s -> %s", topic, topicQosPolicy);

        return topic;
    }

    @Override
    public String toString() {
        return getTopicName();
    }
}
