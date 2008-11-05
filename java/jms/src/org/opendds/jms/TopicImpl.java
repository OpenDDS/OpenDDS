/*
 * $Id$
 */

package org.opendds.jms;

import java.io.Serializable;
import java.util.Properties;

import javax.jms.Topic;

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
        return getTopicName();
    }
}
