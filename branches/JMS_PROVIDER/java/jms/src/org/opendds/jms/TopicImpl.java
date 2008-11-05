/*
 * $Id$
 */

package org.opendds.jms;

import java.io.Serializable;
import java.util.Properties;

import javax.jms.Topic;
import javax.jms.Destination;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TopicImpl implements Serializable, Topic {
    private String topicName;

    private Properties dataReaderQosPolicy;
    private Properties dataWriterQosPolicy;
    private Properties topicQosPolicy;

    public TopicImpl(String topicName) {
        this.topicName = topicName;
    }

    public String getTopicName() {
        return topicName;
    }

    public Properties getDataReaderQosPolicy() {
        return dataReaderQosPolicy;
    }

    public void setDataReaderQosPolicy(Properties dataReaderQosPolicy) {
        this.dataReaderQosPolicy = dataReaderQosPolicy;
    }

    public Properties getDataWriterQosPolicy() {
        return dataWriterQosPolicy;
    }

    public void setDataWriterQosPolicy(Properties dataWriterQosPolicy) {
        this.dataWriterQosPolicy = dataWriterQosPolicy;
    }

    public Properties getTopicQosPolicy() {
        return topicQosPolicy;
    }

    public void setTopicQosPolicy(Properties topicQosPolicy) {
        this.topicQosPolicy = topicQosPolicy;
    }

    public DDS.Topic createTopic() {
        throw new UnsupportedOperationException();
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder(getClass().getName());
        sb.append("[topicName=" + topicName + "]");
        return sb.toString();
    }

    static Destination fromTopicName(String topicName) {
        return new TopicImpl(topicName);
    }

}
