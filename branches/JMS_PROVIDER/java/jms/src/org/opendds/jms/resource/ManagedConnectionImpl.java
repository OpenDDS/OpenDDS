/*
 * $Id$
 */

package org.opendds.jms.resource;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

import javax.resource.NotSupportedException;
import javax.resource.ResourceException;
import javax.resource.spi.ConnectionEventListener;
import javax.resource.spi.ConnectionRequestInfo;
import javax.resource.spi.LocalTransaction;
import javax.resource.spi.ManagedConnection;
import javax.resource.spi.ManagedConnectionMetaData;
import javax.security.auth.Subject;
import javax.transaction.xa.XAResource;

import DDS.DomainParticipant;
import DDS.DomainParticipantFactory;
import DDS.DomainParticipantQosHolder;
import DDS.RETCODE_OK;
import OpenDDS.DCPS.TheParticipantFactory;
import OpenDDS.JMS.MessagePayloadTypeSupportImpl;

import org.opendds.jms.PublisherManager;
import org.opendds.jms.SubscriberManager;
import org.opendds.jms.common.Version;
import org.opendds.jms.common.lang.Objects;
import org.opendds.jms.qos.ParticipantQosPolicy;
import org.opendds.jms.qos.QosPolicies;

/**
 * @author Steven Stallion
 * @version $Revision$
 */
public class ManagedConnectionImpl implements ManagedConnection {
    private Subject subject;
    private ConnectionRequestInfoImpl cxRequestInfo;
    private DomainParticipant participant;
    private String typeName;
    private PublisherManager publishers;
    private SubscriberManager subscribers;
    private PrintWriter out;

    private List<ConnectionEventListener> listeners =
        new ArrayList<ConnectionEventListener>();

    public ManagedConnectionImpl(Subject subject,
                                 ConnectionRequestInfoImpl cxRequestInfo) throws ResourceException {
        this.subject = subject;
        this.cxRequestInfo = cxRequestInfo;

        create();
    }

    public Subject getSubject() {
        return subject;
    }

    public ConnectionRequestInfoImpl getConnectionRequestInfo() {
        return cxRequestInfo;
    }

    public String getConnectionId() {
        return null;
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
        listeners.add(listener);
    }

    public void removeConnectionEventListener(ConnectionEventListener listener) {
        listeners.remove(listener);
    }

    public void associateConnection(Object o) throws ResourceException {
    }

    public Object getConnection(Subject subject, ConnectionRequestInfo requestInfo) throws ResourceException {
        return null;
    }

    public void create() throws ResourceException {
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
        if (ts.register_type(participant, null) != RETCODE_OK.value) {
            throw new ResourceException("Unable to register type; please check logs");
        }
        typeName = ts.get_type_name();

        publishers = new PublisherManager(this);
        subscribers = new SubscriberManager(this);
    }

    public void destroy() throws ResourceException {
    }

    public void cleanup() throws ResourceException {
    }

    public XAResource getXAResource() throws ResourceException {
        throw new NotSupportedException(); // transactions not supported
    }

    public LocalTransaction getLocalTransaction() throws ResourceException {
        throw new NotSupportedException(); // transactions not supported
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
