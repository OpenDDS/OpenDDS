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

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ManagedConnectionFactoryImpl implements ManagedConnectionFactory {
    private String participantQosPolicy;
    private String publisherQosPolicy;
    private String subscriberQoSPolicy;
    private String transportConfiguration;

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

    public Object createConnectionFactory(ConnectionManager connectionManager) throws ResourceException {
        return null;
    }

    public Object createConnectionFactory() throws ResourceException {
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

    public void setLogWriter(PrintWriter printWriter) throws ResourceException {}

    public PrintWriter getLogWriter() throws ResourceException {
        return null;
    }
}
