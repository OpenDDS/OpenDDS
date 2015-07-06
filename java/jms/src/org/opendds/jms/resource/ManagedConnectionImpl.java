/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.resource;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

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
import OpenDDS.DCPS.DEFAULT_STATUS_MASK;
import OpenDDS.DCPS.TheParticipantFactory;
import OpenDDS.JMS.MessagePayloadTypeSupportImpl;

import org.opendds.jms.ConnectionImpl;
import org.opendds.jms.PublisherManager;
import org.opendds.jms.SubscriberManager;
import org.opendds.jms.common.Version;
import org.opendds.jms.common.lang.Objects;
import org.opendds.jms.common.util.Logger;
import org.opendds.jms.qos.ParticipantQosPolicy;
import org.opendds.jms.qos.QosPolicies;

/**
 * @author Steven Stallion
 */
public class ManagedConnectionImpl implements ManagedConnection {
    private Logger logger;

    private boolean destroyed;
    private Subject subject;
    private ConnectionRequestInfoImpl cxRequestInfo;
    private DomainParticipantFactory dpf;
    private DomainParticipant participant;
    private String typeName;
    private PublisherManager publisherManager;
    private SubscriberManager subscriberManager;

    private List<ConnectionImpl> handles =
        new ArrayList<ConnectionImpl>();

    private List<ConnectionEventListener> listeners =
        new ArrayList<ConnectionEventListener>();

    public ManagedConnectionImpl(Subject subject,
                                 ConnectionRequestInfoImpl cxRequestInfo) throws ResourceException {

        logger = Logger.getLogger("Domain", cxRequestInfo.getDomainID());

        this.subject = subject;
        this.cxRequestInfo = cxRequestInfo;

        dpf = TheParticipantFactory.getInstance();
        if (dpf == null) {
            throw new ResourceException("Unable to get DomainParticipantFactory instance; please check logs");
        }
        logger.debug("Using %s", dpf);

        DomainParticipantQosHolder holder =
            new DomainParticipantQosHolder(QosPolicies.newParticipantQos());

        dpf.get_default_participant_qos(holder);

        ParticipantQosPolicy policy = cxRequestInfo.getParticipantQosPolicy();
        policy.setQos(holder.value);

        participant = dpf.create_participant(cxRequestInfo.getDomainID(),
                                             holder.value,
                                             null,
                                             DEFAULT_STATUS_MASK.value);
        if (participant == null) {
            throw new ResourceException("Unable to create DomainParticipant; please check logs");
        }
        logger.debug("Created %s -> %s", participant, policy);

        logger.debug("Connection ID is %s", getConnectionId());

        MessagePayloadTypeSupportImpl ts = new MessagePayloadTypeSupportImpl();
        if (ts.register_type(participant, "") != RETCODE_OK.value) {
            throw new ResourceException("Unable to register type; please check logs");
        }
        typeName = ts.get_type_name();
        logger.debug("Registered %s", typeName);

        publisherManager = new PublisherManager(this);
        logger.debug("Created %s", publisherManager);

        subscriberManager = new SubscriberManager(this);
        logger.debug("Created %s", subscriberManager);
    }

    public Logger getLogger() {
        return logger;
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
        return OpenDDS.DCPS.TheServiceParticipant.get_unique_id(participant);
    }

    public DomainParticipant getParticipant() {
        return participant;
    }

    public String getTypeName() {
        return typeName;
    }

    public PublisherManager getPublisherManager() {
        return publisherManager;
    }

    public SubscriberManager getSubscriberManager() {
        return subscriberManager;
    }

    public PrintWriter getLogWriter() {
        return null;
    }

    public void setLogWriter(PrintWriter out) {}

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
        handle.setManagedConnection(this);

        synchronized (handles) {
            handles.add(handle);
        }
        logger.debug("Associated %s", handle);
    }

    public Object getConnection(Subject subject,
                                ConnectionRequestInfo cxRequestInfo) throws ResourceException {
        checkDestroyed();

        ConnectionImpl handle = new ConnectionImpl(this);

        synchronized (handles) {
            handles.add(handle);
        }
        logger.debug("Created %s", handle);

        return handle; // re-configuration not supported
    }

    public XAResource getXAResource() throws ResourceException {
        throw new NotSupportedException(); // transactions not supported
    }

    public LocalTransaction getLocalTransaction() throws ResourceException {
        throw new NotSupportedException(); // transactions not supported
    }

    public boolean matches(Subject subject, ConnectionRequestInfo cxRequestInfo) {
        return Objects.equals(this.subject, subject)
            && Objects.equals(this.cxRequestInfo, cxRequestInfo);
    }

    public synchronized void cleanup() throws ResourceException {
        checkDestroyed();

        logger.debug("Cleaning up %s", this);

        synchronized (handles) {
            for (ConnectionImpl handle : handles) {
                handle.close(false);
            }
            handles.clear();
        }
    }

    public synchronized void destroy() throws ResourceException {
        checkDestroyed();

        logger.debug("Destroying %s", this);

        cleanup();

        participant.delete_contained_entities();
        dpf.delete_participant(participant);
        logger.debug("Deleted %s", participant);

        subject = null;
        cxRequestInfo = null;
        dpf = null;
        participant = null;
        typeName = null;
        publisherManager = null;
        subscriberManager = null;

        destroyed = true;
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

    protected void checkDestroyed() throws ResourceException {
        if (isDestroyed()) {
            throw new IllegalStateException();
        }
    }

    public void notifyClosed(ConnectionImpl handle) {
        ConnectionEvent event = new ConnectionEvent(this, ConnectionEvent.CONNECTION_CLOSED);
        event.setConnectionHandle(handle);

        notifyListeners(event);
    }

    public void notifyError(Exception e) {
        notifyListeners(new ConnectionEvent(this, ConnectionEvent.CONNECTION_ERROR_OCCURRED, e));
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
}
