/*
 * $Id$
 */

package org.opendds.jms.resource;

import java.io.PrintWriter;
import java.util.Set;

import javax.resource.ResourceException;
import javax.resource.spi.ConnectionManager;
import javax.resource.spi.ConnectionRequestInfo;
import javax.resource.spi.ManagedConnection;
import javax.resource.spi.ManagedConnectionFactory;
import javax.security.auth.Subject;

import org.opendds.jms.ConnectionFactoryImpl;
import org.opendds.jms.common.lang.Objects;
import org.opendds.jms.qos.ParticipantQosPolicy;
import org.opendds.jms.qos.PublisherQosPolicy;
import org.opendds.jms.qos.SubscriberQosPolicy;
import org.opendds.jms.transport.TransportConfigurationFactory;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ManagedConnectionFactoryImpl implements ManagedConnectionFactory {
    private Integer domainId;
    private String participantQosPolicy;
    private String publisherQosPolicy;
    private String publisherTransport;
    private String subscriberQosPolicy;
    private String subscriberTransport;
    private String transportType;

    private PrintWriter log;

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
        return log;
    }

    public void setLogWriter(PrintWriter log) {
        this.log = log;
    }

    public Object createConnectionFactory() {
        return createConnectionFactory(null);
    }

    public Object createConnectionFactory(ConnectionManager connectionManager) {
        TransportConfigurationFactory configFactory =
            new TransportConfigurationFactory(transportType);

        ConnectionFactoryImpl instance = new ConnectionFactoryImpl(connectionManager);

        instance.setDomainId(domainId);
        instance.setParticipantQosPolicy(new ParticipantQosPolicy(participantQosPolicy));
        instance.setPublisherQosPolicy(new PublisherQosPolicy(publisherQosPolicy));
        instance.setPublisherTransport(configFactory.createConfiguration(publisherTransport));
        instance.setSubscriberQosPolicy(new SubscriberQosPolicy(subscriberQosPolicy));
        instance.setSubscriberTransport(configFactory.createConfiguration(subscriberTransport));

        return instance;
    }

    public ManagedConnection createManagedConnection(Subject subject,
                                                     ConnectionRequestInfo requestInfo) throws ResourceException {
        return null;
    }

    public ManagedConnection matchManagedConnections(Set set,
                                                     Subject subject,
                                                     ConnectionRequestInfo requestInfo) throws ResourceException {
        return null;
    }

    @Override
    public int hashCode() {
        return Objects.hashCode(domainId,
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

        ManagedConnectionFactoryImpl cf = (ManagedConnectionFactoryImpl) o;
        return Objects.equalsWithNull(domainId, cf.domainId)
            || Objects.equalsWithNull(participantQosPolicy, cf.participantQosPolicy)
            || Objects.equalsWithNull(publisherQosPolicy, cf.publisherQosPolicy)
            || Objects.equalsWithNull(publisherTransport, cf.publisherTransport)
            || Objects.equalsWithNull(subscriberQosPolicy, cf.subscriberQosPolicy)
            || Objects.equalsWithNull(subscriberTransport, cf.subscriberTransport)
            || Objects.equalsWithNull(transportType, cf.transportType);
    }
}
