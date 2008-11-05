/*
 * $Id$
 */

package org.opendds.jms;

import java.util.List;
import java.util.ArrayList;
import javax.jms.Connection;
import javax.jms.ConnectionConsumer;
import javax.jms.ConnectionMetaData;
import javax.jms.Destination;
import javax.jms.ExceptionListener;
import javax.jms.JMSException;
import javax.jms.ServerSessionPool;
import javax.jms.Session;
import javax.jms.Topic;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ConnectionImpl implements Connection {
    private List<TemporaryTopicImpl> temporaryTopics;

    public ConnectionImpl() {
        this.temporaryTopics = new ArrayList<TemporaryTopicImpl>();
    }

    public String getClientID() throws JMSException {
        return null;
    }

    public void setClientID(String clientID) throws JMSException {
    }

    public ExceptionListener getExceptionListener() throws JMSException {
        return null;
    }

    public void setExceptionListener(ExceptionListener listener) throws JMSException {
    }

    public ConnectionMetaData getMetaData() throws JMSException {
        return null;
    }

    public void start() throws JMSException {
    }

    public ConnectionConsumer createConnectionConsumer(Destination destination,
                                                       String messageSelector,
                                                       ServerSessionPool sessionPool,
                                                       int maxMessages) throws JMSException {
        throw new UnsupportedOperationException();
    }

    public ConnectionConsumer createDurableConnectionConsumer(Topic topic,
                                                              String subscriptionName,
                                                              String messageSelector,
                                                              ServerSessionPool sessionPool,
                                                              int maxMessages) throws JMSException {
        throw new UnsupportedOperationException();
    }

    public Session createSession(boolean transacted, int acknowledgeMode) throws JMSException {
        return null;
    }

    public void stop() throws JMSException {
    }

    public void close() throws JMSException {
    }

    public void addTemporaryTopic(TemporaryTopicImpl temporaryTopic) {
        this.temporaryTopics.add(temporaryTopic);
    }

    public void removeTemporaryTopic(TemporaryTopicImpl temporaryTopic) {
        this.temporaryTopics.remove(temporaryTopic);
    }
}
