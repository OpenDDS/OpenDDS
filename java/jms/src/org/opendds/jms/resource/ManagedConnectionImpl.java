/*
 * $Id$
 */

package org.opendds.jms.resource;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

import javax.jms.JMSException;
import javax.resource.NotSupportedException;
import javax.resource.ResourceException;
import javax.resource.spi.ConnectionEvent;
import javax.resource.spi.ConnectionEventListener;
import javax.resource.spi.ConnectionRequestInfo;
import javax.resource.spi.IllegalStateException;
import javax.resource.spi.LocalTransaction;
import javax.resource.spi.ManagedConnection;
import javax.resource.spi.ManagedConnectionMetaData;
import javax.security.auth.Subject;
import javax.transaction.xa.XAResource;

import DDS.DomainParticipant;
import DDS.DomainParticipantFactory;
import DDS.DomainParticipantQosHolder;
import DDS.RETCODE_OK;
import OpenDDS.DCPS.DomainParticipantExt;
import OpenDDS.DCPS.DomainParticipantExtHelper;
import OpenDDS.DCPS.TheParticipantFactory;
import OpenDDS.JMS.MessagePayloadTypeSupportImpl;

import org.opendds.jms.ConnectionImpl;
import org.opendds.jms.PublisherManager;
import org.opendds.jms.SubscriberManager;
import org.opendds.jms.common.Version;
import org.opendds.jms.qos.ParticipantQosPolicy;
import org.opendds.jms.qos.QosPolicies;

/**
 * @author Steven Stallion
 * @version $Revision$
 */
public class ManagedConnectionImpl implements ManagedConnection {
    private boolean destroyed;
    private Subject subject;
    private ConnectionRequestInfoImpl cxRequestInfo;
    private DomainParticipant participant;
    private String typeName;
    private PublisherManager publishers;
    private SubscriberManager subscribers;
    private PrintWriter out;

    private List<ConnectionImpl> handles =
        new ArrayList<ConnectionImpl>();

    private List<ConnectionEventListener> listeners =
        new ArrayList<ConnectionEventListener>();

    public ManagedConnectionImpl(Subject subject,
                                 ConnectionRequestInfoImpl cxRequestInfo) throws ResourceException {
        this.subject = subject;
        this.cxRequestInfo = cxRequestInfo;

        DomainParticipantFactory dpf = TheParticipantFactory.getInstance();
        if (dpf == null) {
            throw new ResourceException("Unable to get DomainParticipantFactory instance; please check logs");
        }

        DomainParticipantQosHolder holder =
            new DomainParticipantQosHolder(QosPolicies.newParticipantQos());

        dpf.get_default_participant_qos(holder);

        ParticipantQosPolicy policy = cxRequestInfo.getParticipantQosPolicy();
        policy.setQos(holder.value);

        participant = dpf.create_participant(cxRequestInfo.getDomainId(), holder.value, null);
        if (participant == null) {
            throw new ResourceException("Unable to create DomainParticipant; please check logs");
        }

        MessagePayloadTypeSupportImpl ts = new MessagePayloadTypeSupportImpl();
        if (ts.register_type(participant, "") != RETCODE_OK.value) {
            throw new ResourceException("Unable to register type; please check logs");
        }
        typeName = ts.get_type_name();

        publishers = new PublisherManager(this);
        subscribers = new SubscriberManager(this);
    }

    public boolean isDestroyed() {
        return destroyed;
    }

    public Subject getSubject() {
        return subject;
    }

    public ConnectionRequestInfoImpl getConnectionRequestInfo() {
        return cxRequestInfo;
    }

    public String getConnectionId() {
        DomainParticipantExt ext = DomainParticipantExtHelper.narrow(participant);
        return String.format("%08x%08x", ext.get_federation_id(), ext.get_participant_id());
    }

    public DomainParticipant getParticipant() {
        return participant;
    }

    public String getTypeName() {
        return typeName;
    }

    public PublisherManager getPublishers() {
        return publishers;
    }

    public SubscriberManager getSubscribers() {
        return subscribers;
    }

    public PrintWriter getLogWriter() {
        return out;
    }

    public void setLogWriter(PrintWriter out) {
        this.out = out;
    }

    public void addConnectionEventListener(ConnectionEventListener listener) {
        synchronized (listeners) {
            listeners.add(listener);
        }
    }

    public void removeConnectionEventListener(ConnectionEventListener listener) {
        synchronized (listeners) {
            listeners.remove(listener);
        }
    }

    public void associateConnection(Object o) throws ResourceException {
        checkDestroyed();

        if (!(o instanceof ConnectionImpl)) {
            throw new IllegalArgumentException();
        }

        ConnectionImpl handle = (ConnectionImpl) o;
        handle.setParent(this);

        synchronized (handles) {
            handles.add(handle);
        }
    }

    public Object getConnection(Subject subject,
                                ConnectionRequestInfo cxRequestInfo) throws ResourceException {
        checkDestroyed();

        ConnectionImpl handle = new ConnectionImpl(this);

        synchronized (handles) {
            handles.add(handle);
        }

        return handle; // re-configuration not supported
    }

    public XAResource getXAResource() throws ResourceException {
        throw new NotSupportedException(); // transactions not supported
    }

    public LocalTransaction getLocalTransaction() throws ResourceException {
        throw new NotSupportedException(); // transactions not supported
    }

    public synchronized void cleanup() throws ResourceException {
        checkDestroyed();

        synchronized (handles) {
            for (ConnectionImpl handle : handles) {
                if (!handle.isClosed()) {
                    try {
                        handle.close();

                    } catch (JMSException e) {}
                }
            }
            handles.clear();
        }
    }

    public synchronized void destroy() throws ResourceException {
        checkDestroyed();

        cleanup();

        participant.delete_contained_entities();
        notifyListeners(new ConnectionEvent(this, ConnectionEvent.CONNECTION_CLOSED));

        subject = null;
        cxRequestInfo = null;
        participant = null;
        typeName = null;
        publishers = null;
        subscribers = null;

        destroyed = true;
    }

    protected void checkDestroyed() throws ResourceException {
        if (isDestroyed()) {
            throw new IllegalStateException();
        }
    }

    protected void notifyListeners(ConnectionEvent event) {
        synchronized (listeners) {
            for (ConnectionEventListener listener : listeners) {
                switch (event.getId()) {
                    case ConnectionEvent.CONNECTION_CLOSED:
                        listener.connectionClosed(event);
                        break;

                    case ConnectionEvent.LOCAL_TRANSACTION_STARTED:
                        listener.localTransactionStarted(event);
                        break;

                    case ConnectionEvent.LOCAL_TRANSACTION_COMMITTED:
                        listener.localTransactionCommitted(event);
                        break;

                    case ConnectionEvent.LOCAL_TRANSACTION_ROLLEDBACK:
                        listener.localTransactionRolledback(event);
                        break;

                    case ConnectionEvent.CONNECTION_ERROR_OCCURRED:
                        listener.connectionErrorOccurred(event);
                        break;
                }
            }
        }
    }

    public ManagedConnectionMetaData getMetaData() throws ResourceException {
        return new ManagedConnectionMetaData() {
            private Version version = Version.getInstance();

            public String getEISProductName() {
                return version.getProductName();
            }

            public String getEISProductVersion() {
                return version.getDDSVersion();
            }

            public int getMaxConnections() {
                return 0;
            }

            public String getUserName() {
                return null; // authentication not supported
            }
        };
    }
}
