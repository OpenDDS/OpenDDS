/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

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
import org.opendds.jms.common.lang.Strings;
import org.opendds.jms.common.util.Logger;
import org.opendds.jms.persistence.PersistenceManager;
import org.opendds.jms.resource.ConnectionRequestInfoImpl;
import org.opendds.jms.resource.ManagedConnectionImpl;

/**
 * @author  Steven Stallion
 */
public class ConnectionImpl implements Connection {
    private Logger logger;

    private boolean closed;
    private boolean started;
    private ManagedConnectionImpl connection;
    private DomainParticipant participant;
    private PublisherManager publisherManager;
    private SubscriberManager subscriberManager;
    private String clientID;
    private PersistenceManager persistenceManager;
    private ExceptionListener exceptionListener;

    private List<ConnectionStateListener> stateListeners =
        new ArrayList<ConnectionStateListener>();

    private List<Session> sessions =
        new ArrayList<Session>();

    private List<TemporaryTopic> tempTopics =
        new ArrayList<TemporaryTopic>();

    private Map<String, DurableMessageConsumerImpl> durableSubscriptions;
    final private Object lockForurableSubscriptions;

    public ConnectionImpl(ManagedConnectionImpl connection) {
        assert connection != null;

        this.connection = connection;

        this.logger = connection.getLogger();
        this.participant = connection.getParticipant();
        this.publisherManager = connection.getPublisherManager();
        this.subscriberManager = connection.getSubscriberManager();

        ConnectionRequestInfoImpl cxRequestInfo = connection.getConnectionRequestInfo();
        this.clientID = cxRequestInfo.getClientID();

        this.persistenceManager = cxRequestInfo.getPersistenceManager();
        if (persistenceManager == null) {
            logger.warn("PersistenceManager not bound; durable subscriptions are unavailable");
        }

        this.durableSubscriptions = new HashMap<String, DurableMessageConsumerImpl>();
        this.lockForurableSubscriptions = new Object();
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
            throw new JMSException("PersistenceManager not bound; durable subscriptions are unavailable");
        }
        return persistenceManager;
    }

    public String getTypeName() {
        return connection.getTypeName();
    }

    public Publisher getPublisher() throws JMSException {
        try {
            return publisherManager.getPublisher();

        } catch (JMSException e) {
            throw notifyExceptionListener(e);
        }
    }

    public Subscriber getLocalSubscriber() throws JMSException {
        try {
            return subscriberManager.getLocalSubscriber();

        } catch (JMSException e) {
            throw notifyExceptionListener(e);
        }
    }

    public Subscriber getRemoteSubscriber() throws JMSException {
        try {
            return subscriberManager.getRemoteSubscriber();

        } catch (JMSException e) {
            throw notifyExceptionListener(e);
        }
    }

    public String getClientID() {
        return clientID;
    }

    public void setClientID(String clientID) {
        if (!Strings.isEmpty(this.clientID)) {
            throw new IllegalStateException("The Client ID has been administratively specified.");
        }
        this.clientID = clientID;
    }

    public ExceptionListener getExceptionListener() {
        return exceptionListener;
    }

    public void setExceptionListener(ExceptionListener listener) {
        this.exceptionListener = listener;
    }

    protected JMSException notifyExceptionListener(JMSException e) {
        if (exceptionListener != null) {
            exceptionListener.onException(e);
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
        if (logger.isDebugEnabled()) {
            logger.debug("Created %s", Strings.asIdentity(topic));
        }

        return topic;
    }

    protected void deleteTemporaryTopic(TemporaryTopic topic) {
        synchronized (tempTopics) {
            tempTopics.remove(topic);
        }
    }

    void registerDurableSubscription(String name, DurableMessageConsumerImpl durableSubscription) {
        synchronized(lockForurableSubscriptions) {
            durableSubscriptions.put(name, durableSubscription);
        }
    }

    DurableMessageConsumerImpl getDurableSubscriptionByname(String name) {
        synchronized (lockForurableSubscriptions) {
            return durableSubscriptions.get(name);
        }
    }

    boolean hasDurableSubscription(String name) {
        synchronized(lockForurableSubscriptions) {
            return durableSubscriptions.containsKey(name);
        }
    }

    void removeDurableSubscription(String name) {
        synchronized(lockForurableSubscriptions) {
            durableSubscriptions.remove(name);
        }
    }

    void unregisterDurableSubscription(String name) {
        synchronized(lockForurableSubscriptions) {
            durableSubscriptions.put(name, null);
        }
    }

    void unsubscribeDurableSubscription(String name) {
        synchronized(lockForurableSubscriptions) {
            durableSubscriptions.remove(name);
        }
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
            int size = tempTopics.size();
            for (int i = size - 1; i >= 0; i--) {
                TemporaryTopic topic = tempTopics.get(i);
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

    public void addStateListener(ConnectionStateListener stateListener) {
        synchronized (stateListeners) {
            // notify listener of current state
            if (isStarted()) {
                notifyConnectionStarted(stateListener);
            } else {
                notifyConnectionStopped(stateListener);
            }
            stateListeners.add(stateListener);
        }
    }

    public void removeStateListener(ConnectionStateListener stateListener) {
        synchronized (stateListeners) {
            stateListeners.remove(stateListener);
        }
    }

    public boolean isStarted() {
        return started;
    }

    public synchronized void start() throws JMSException {
        if (isStarted()) {
            return; // ignore
        }

        notifyStateListeners(ConnectionStateEvent.CONNECTION_STARTED);
        started = true;
    }

    public synchronized void stop() throws JMSException {
        if (!isStarted()) {
            return; // ignore
        }

        notifyStateListeners(ConnectionStateEvent.CONNECTION_STOPPED);
        started = false;
    }

    protected void notifyStateListeners(int eventId) {
        synchronized (stateListeners) {
            for (ConnectionStateListener stateListener : stateListeners) {
                switch (eventId) {
                    case ConnectionStateEvent.CONNECTION_STARTED:
                        notifyConnectionStarted(stateListener);
                        break;

                    case ConnectionStateEvent.CONNECTION_STOPPED:
                        notifyConnectionStopped(stateListener);
                        break;
                }
            }
        }
    }

    protected void notifyConnectionStarted(ConnectionStateListener stateListener) {
        stateListener.connectionStarted(new ConnectionStateEvent(this, ConnectionStateEvent.CONNECTION_STARTED));
    }

    protected void notifyConnectionStopped(ConnectionStateListener stateListener) {
        stateListener.connectionStarted(new ConnectionStateEvent(this, ConnectionStateEvent.CONNECTION_STOPPED));
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
                List<String> names = new ArrayList<String>();

                names.add("JMSXGroupID");  // required (see JMS 3.5.9)
                names.add("JMSXGroupSeq"); // required (see JMS 3.5.9)

                return Collections.enumeration(names);
            }
        };
    }
}
