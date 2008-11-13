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
    private String participantQosPolicy;
    private String publisherQosPolicy;
    private String subscriberQoSPolicy;
    private String transportConfiguration;

    private PrintWriter out;

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

    public String getSubscriberQoSPolicy() {
        return subscriberQoSPolicy;
    }

    public void setSubscriberQoSPolicy(String subscriberQoSPolicy) {
        this.subscriberQoSPolicy = subscriberQoSPolicy;
    }

    public String getTransportConfiguration() {
        return transportConfiguration;
    }

    public void setTransportConfiguration(String transportConfiguration) {
        this.transportConfiguration = transportConfiguration;
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
        return Objects.hashCode(participantQosPolicy,
                                publisherQosPolicy,
                                subscriberQoSPolicy,
                                transportConfiguration);
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
        return Objects.equalsWithNull(participantQosPolicy, cf.participantQosPolicy)
            || Objects.equalsWithNull(publisherQosPolicy, cf.publisherQosPolicy)
            || Objects.equalsWithNull(subscriberQoSPolicy, cf.subscriberQoSPolicy)
            || Objects.equalsWithNull(transportConfiguration, cf.transportConfiguration);
    }
}
