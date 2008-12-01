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
import org.opendds.jms.common.util.Logger;
import org.opendds.jms.persistence.PersistenceManager;
import org.opendds.jms.resource.ConnectionRequestInfoImpl;
import org.opendds.jms.resource.ManagedConnectionImpl;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ConnectionImpl implements Connection {
    private Logger logger;

    private ManagedConnectionImpl connection;
    private DomainParticipant participant;
    private PublisherManager publishers;
    private SubscriberManager subscribers;
    private String clientId;
    private PersistenceManager persistenceManager;
    private boolean closed;
    private ExceptionListener listener;

    private List<Session> sessions =
        new ArrayList<Session>();

    private List<TemporaryTopic> tempTopics =
        new ArrayList<TemporaryTopic>();

    public ConnectionImpl(ManagedConnectionImpl connection) {
        this.connection = connection;

        logger = connection.getLogger();
        participant = connection.getParticipant();
        publishers = connection.getPublishers();
        subscribers = connection.getSubscribers();

        ConnectionRequestInfoImpl cxRequestInfo = connection.getConnectionRequestInfo();
        clientId = cxRequestInfo.getClientID();

        persistenceManager = cxRequestInfo.getPersistenceManager();
        if (persistenceManager == null) {
            logger.warn("PersistenceManager not set; durable subscriptions unavailable");
        }
    }

    public ManagedConnectionImpl getManagedConnection() {
        return connection;
    }

    public void setManagedConnection(ManagedConnectionImpl connection) {
        this.connection = connection;
    }

    public Logger getLogger() {
        return logger;
    }

    public DomainParticipant getParticipant() {
        return participant;
    }

    public PersistenceManager getPersistenceManager() throws JMSException {
        if (persistenceManager == null) {
            throw new JMSException("PersistenceManager not set; durable subscriptions unavailable");
        }
        return persistenceManager;
    }

    public String getTypeName() {
        return connection.getTypeName();
    }

    public Publisher getPublisher() throws JMSException {
        try {
            return publishers.getPublisher();

        } catch (JMSException e) {
            throw notifyExceptionListener(e);
        }
    }

    public Subscriber getLocalSubscriber() throws JMSException {
        try {
            return subscribers.getLocalSubscriber();

        } catch (JMSException e) {
            throw notifyExceptionListener(e);
        }
    }

    public Subscriber getRemoteSubscriber() throws JMSException {
        try {
            return subscribers.getRemoteSubscriber();

        } catch (JMSException e) {
            throw notifyExceptionListener(e);
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
        SessionImpl session = new SessionImpl(this, transacted, acknowledgeMode);

        synchronized (sessions) {
            sessions.add(session);
        }
        logger.debug("Created %s", session);

        return session;
    }

    protected TemporaryTopic createTemporaryTopic() {
        TemporaryTopicImpl topic = new TemporaryTopicImpl(this);

        synchronized (tempTopics) {
            tempTopics.add(topic);
        }
        logger.debug("Created %s", topic);

        return topic;
    }

    protected void deleteTemporaryTopic(TemporaryTopic topic) {
        synchronized (tempTopics) {
            tempTopics.remove(topic);
        }
    }

    public synchronized void start() throws JMSException {
    }

    public synchronized void stop() throws JMSException {
    }

    public boolean isClosed() {
        return closed;
    }

    public synchronized void close() {
        close(true);
    }

    public synchronized void close(boolean notify) {
        if (isClosed()) {
            return;
        }

        logger.debug("Closing %s", this);

        synchronized (sessions) {
            for (Session session : sessions) {
                try {
                    session.close();

                } catch (JMSException e) {}
            }
            sessions.clear();
        }

        synchronized (tempTopics) {
            for (TemporaryTopic topic : tempTopics) {
                try {
                    topic.delete();

                } catch (JMSException e) {}
            }
            tempTopics.clear();
        }

        closed = true;

        if (notify) {
            connection.notifyClosed(this);
        }
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

    protected JMSException notifyExceptionListener(JMSException e) {
        if (listener != null) {
            listener.onException(e);
        }
        return e;
    }
}
