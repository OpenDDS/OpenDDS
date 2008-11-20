/*
 * $Id$
 */

package org.opendds.jms;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.List;

import javax.jms.Connection;
import javax.jms.ConnectionConsumer;
import javax.jms.ConnectionMetaData;
import javax.jms.Destination;
import javax.jms.ExceptionListener;
import javax.jms.JMSException;
import javax.jms.ServerSessionPool;
import javax.jms.Session;
import javax.jms.TemporaryTopic;
import javax.jms.Topic;

import DDS.DomainParticipant;
import DDS.Publisher;
import DDS.Subscriber;

import org.opendds.jms.common.Version;
import org.opendds.jms.resource.ConnectionRequestInfoImpl;
import org.opendds.jms.resource.ManagedConnectionImpl;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ConnectionImpl implements Connection {
    private ManagedConnectionImpl connection;
    private DomainParticipant participant;
    private PublisherManager publishers;
    private SubscriberManager subscribers;
    private String clientId;
    private boolean closed;
    private ExceptionListener listener;

    private List<Session> sessions =
        new ArrayList<Session>();

    private List<TemporaryTopicImpl> tempTopics =
        new ArrayList<TemporaryTopicImpl>();

    public ConnectionImpl(ManagedConnectionImpl connection) {
        this.connection = connection;

        participant = connection.getParticipant();
        publishers = connection.getPublishers();
        subscribers = connection.getSubscribers();

        ConnectionRequestInfoImpl cxRequestInfo = connection.getConnectionRequestInfo();
        clientId = cxRequestInfo.getClientId();
    }

    public ManagedConnectionImpl getManagedConnection() {
        return connection;
    }

    public void setManagedConnection(ManagedConnectionImpl connection) {
        this.connection = connection;
    }

    public DomainParticipant getParticipant() {
        return participant;
    }

    public Publisher getPublisher() throws JMSException {
        try {
            return publishers.getPublisher();

        } catch (JMSException e) {
            throw notifyListener(e);
        }
    }

    public Subscriber getLocalSubscriber() throws JMSException {
        try {
            return subscribers.getLocalSubscriber();

        } catch (JMSException e) {
            throw notifyListener(e);
        }
    }

    public Subscriber getRemoteSubscriber() throws JMSException {
        try {
            return subscribers.getRemoteSubscriber();

        } catch (JMSException e) {
            throw notifyListener(e);
        }
    }

    public String getClientID() {
        return clientId;
    }

    public void setClientID(String clientId) {
        this.clientId = clientId;
    }

    public ExceptionListener getExceptionListener() {
        return listener;
    }

    public void setExceptionListener(ExceptionListener listener) {
        this.listener = listener;
    }

    protected JMSException notifyListener(JMSException e) {
        if (listener != null) {
            listener.onException(e);
        }
        return e;
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

    public Session createSession(boolean transacted, int acknowledgeMode) {
//        SessionImpl session = new SessionImpl(this, transacted, acknowledgeMode);
//
//        synchronized (sessions) {
//            sessions.add(session);
//        }
//
//        return session;
        return null;
    }

    public TemporaryTopic createTemporaryTopic() {
        TemporaryTopicImpl topic = new TemporaryTopicImpl();

        synchronized (tempTopics) {
            tempTopics.add(topic);
        }

        return topic;
    }

    public synchronized void stop() throws JMSException {
    }

    public synchronized void start() throws JMSException {
    }

    public boolean isClosed() {
        return closed;
    }
    
    public synchronized void close() {
        if (isClosed()) {
            return;
        }

        synchronized (tempTopics) {
            for (TemporaryTopic topic : tempTopics) {
                try {
                    topic.delete();

                } catch (JMSException e) {}
            }
            tempTopics.clear();
        }
        
        synchronized (sessions) {
            for (Session session : sessions) {
                try {
                    session.close();
                    
                } catch (JMSException e) {}
            }
            sessions.clear();
        }
        
        closed = true;
    }

    public ConnectionMetaData getMetaData() {
        return new ConnectionMetaData() {
            private Version version = Version.getInstance();

            public String getJMSVersion() {
                return version.getJMSVersion();
            }

            public int getJMSMajorVersion() {
                return version.getJMSMajorVersion();
            }

            public int getJMSMinorVersion() {
                return version.getJMSMinorVersion();
            }

            public String getJMSProviderName() {
                return version.getProductName();
            }

            public String getProviderVersion() {
                return version.getDDSVersion();
            }

            public int getProviderMajorVersion() {
                return version.getDDSMajorVersion();
            }

            public int getProviderMinorVersion() {
                return version.getDDSMinorVersion();
            }

            public Enumeration getJMSXPropertyNames() {
                return Collections.enumeration(Collections.emptyList()); // not implemented
            }
        };
    }
}
