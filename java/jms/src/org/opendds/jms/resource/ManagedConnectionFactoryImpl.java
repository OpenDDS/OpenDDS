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

import org.opendds.jms.common.lang.Objects;

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
    private String subscriberTransportConfig;
    private String transportType;

    private PrintWriter out;

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

    public String getSubscriberTransportConfig() {
        return subscriberTransportConfig;
    }

    public void setSubscriberTransportConfig(String subscriberTransportConfig) {
        this.subscriberTransportConfig = subscriberTransportConfig;
    }

    public String getTransportType() {
        return transportType;
    }

    public void setTransportType(String transportType) {
        this.transportType = transportType;
    }

    public PrintWriter getLogWriter() {
        return out;
    }

    public void setLogWriter(PrintWriter out) {
        this.out = out;
    }

    public Object createConnectionFactory() throws ResourceException {
        return null;
    }

    public Object createConnectionFactory(ConnectionManager connectionManager) throws ResourceException {
        return null;
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
        // ManagedConnectionFactory.hashCode() must be defined in
        // terms of its configuration:
        return Objects.hashCode(domainId,
                                participantQosPolicy,
                                publisherQosPolicy,
                                publisherTransport,
                                subscriberQosPolicy,
                                subscriberTransportConfig,
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

        // ManagedConnectionFactory.equals() must be defined in
        // terms of its configuration:
        return Objects.equalsWithNull(domainId, cf.domainId)
            || Objects.equalsWithNull(participantQosPolicy, cf.participantQosPolicy)
            || Objects.equalsWithNull(publisherQosPolicy, cf.publisherQosPolicy)
            || Objects.equalsWithNull(publisherTransport, cf.publisherTransport)
            || Objects.equalsWithNull(subscriberQosPolicy, cf.subscriberQosPolicy)
            || Objects.equalsWithNull(subscriberTransportConfig, cf.subscriberTransportConfig);
    }
}
