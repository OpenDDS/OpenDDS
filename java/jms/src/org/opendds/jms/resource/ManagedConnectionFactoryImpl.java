/*
 * $Id$
 */

package org.opendds.jms.resource;

import java.io.PrintWriter;
import java.util.Iterator;
import java.util.Set;

import javax.resource.ResourceException;
import javax.resource.spi.ConnectionManager;
import javax.resource.spi.ConnectionRequestInfo;
import javax.resource.spi.IllegalStateException;
import javax.resource.spi.ManagedConnection;
import javax.resource.spi.ManagedConnectionFactory;
import javax.security.auth.Subject;

import org.opendds.jms.ConnectionFactoryImpl;
import org.opendds.jms.common.lang.Objects;
import org.opendds.jms.common.lang.Strings;
import org.opendds.jms.qos.ParticipantQosPolicy;
import org.opendds.jms.qos.PublisherQosPolicy;
import org.opendds.jms.qos.SubscriberQosPolicy;
import org.opendds.jms.transport.TransportConfigurationFactory;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ManagedConnectionFactoryImpl implements ManagedConnectionFactory {
    private String clientId;
    private Integer domainId;
    private String participantQosPolicy;
    private String publisherQosPolicy;
    private String publisherTransport;
    private String subscriberQosPolicy;
    private String subscriberTransport;
    private String transportType;

    public String getClientId() {
        return clientId;
    }

    public void setClientId(String clientId) {
        this.clientId = clientId;
    }

    public Integer getDomainId() {
        return domainId;
    }

    public void setDomainId(Integer domainId) {
        this.domainId = domainId;
    }

    public String getParticipantQosPolicy() {
        return participantQosPolicy;
    }

    public void setParticipantQosPolicy(String participantQosPolicy) {
        this.participantQosPolicy = participantQosPolicy;
    }

    public String getPublisherQosPolicy() {
        return publisherQosPolicy;
    }

    public void setPublisherQosPolicy(String publisherQosPolicy) {
        this.publisherQosPolicy = publisherQosPolicy;
    }

    public String getPublisherTransport() {
        return publisherTransport;
    }

    public void setPublisherTransport(String publisherTransport) {
        this.publisherTransport = publisherTransport;
    }

    public String getSubscriberQosPolicy() {
        return subscriberQosPolicy;
    }

    public void setSubscriberQosPolicy(String subscriberQosPolicy) {
        this.subscriberQosPolicy = subscriberQosPolicy;
    }

    public String getSubscriberTransport() {
        return subscriberTransport;
    }

    public void setSubscriberTransport(String subscriberTransport) {
        this.subscriberTransport = subscriberTransport;
    }

    public String getTransportType() {
        return transportType;
    }

    public void setTransportType(String transportType) {
        this.transportType = transportType;
    }

    public PrintWriter getLogWriter() {
        return null; // logging disabled
    }

    public void setLogWriter(PrintWriter log) {}

    public void validate() throws ResourceException {
        if (domainId == null) {
            throw new IllegalStateException("DomainId is a required config-property!");
        }

        if (Strings.isEmpty(transportType)) {
            throw new IllegalStateException("TransportType is a required config-property!");
        }
    }

    public Object createConnectionFactory() throws ResourceException {
        return createConnectionFactory(null);
    }

    public Object createConnectionFactory(ConnectionManager cxManager) throws ResourceException {
        validate();

        TransportConfigurationFactory tcf =
            new TransportConfigurationFactory(transportType);

        ConnectionRequestInfo cxRequestInfo =
            new ConnectionRequestInfoImpl(clientId, domainId,
                new ParticipantQosPolicy(participantQosPolicy),
                new PublisherQosPolicy(publisherQosPolicy),
                tcf.createConfiguration(publisherTransport),
                new SubscriberQosPolicy(subscriberQosPolicy),
                tcf.createConfiguration(subscriberTransport));

        return new ConnectionFactoryImpl(this, cxManager, cxRequestInfo);
    }

    public ManagedConnection createManagedConnection(Subject subject,
                                                     ConnectionRequestInfo cxRequestInfo) throws ResourceException {

        if (!(cxRequestInfo instanceof ConnectionRequestInfoImpl)) {
            throw new IllegalArgumentException();
        }
        return new ManagedConnectionImpl(subject, (ConnectionRequestInfoImpl) cxRequestInfo);
    }

    public ManagedConnection matchManagedConnections(Set connectionSet,
                                                     Subject subject,
                                                     ConnectionRequestInfo cxRequestInfo) throws ResourceException {
        Iterator itr = connectionSet.iterator();
        while (itr.hasNext()) {
            Object o = itr.next();
            if (o instanceof ManagedConnectionImpl) {
                ManagedConnectionImpl connection = (ManagedConnectionImpl) o;
                if (connection.matches(subject, cxRequestInfo)) {
                    return connection;
                }
            }
        }
        return null;
    }

    @Override
    public int hashCode() {
        return Objects.hashCode(
            clientId,
            domainId,
            participantQosPolicy,
            publisherQosPolicy,
            publisherTransport,
            subscriberQosPolicy,
            subscriberTransport,
            transportType);
    }

    @Override
    public boolean equals(Object o) {
        if (o == this) {
            return true;
        }

        if (!(o instanceof ManagedConnectionFactoryImpl)) {
            return false;
        }

        ManagedConnectionFactoryImpl mcf = (ManagedConnectionFactoryImpl) o;
        return Objects.equals(clientId, mcf.clientId)
            && Objects.equals(domainId, mcf.domainId)
            && Objects.equals(participantQosPolicy, mcf.participantQosPolicy)
            && Objects.equals(publisherQosPolicy, mcf.publisherQosPolicy)
            && Objects.equals(publisherTransport, mcf.publisherTransport)
            && Objects.equals(subscriberQosPolicy, mcf.subscriberQosPolicy)
            && Objects.equals(subscriberTransport, mcf.subscriberTransport)
            && Objects.equals(transportType, mcf.transportType);
    }
}
