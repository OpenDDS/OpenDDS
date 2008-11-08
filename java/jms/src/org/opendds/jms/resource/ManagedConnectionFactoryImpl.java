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
    private String domainParticipantQosPolicy;
    private String publisherQosPolicy;
    private String publisherTransport;
    private String subscriberQoSPolicy;
    private String subscriberTransport;

    public String getDomainParticipantQosPolicy() {
        return domainParticipantQosPolicy;
    }

    public void setDomainParticipantQosPolicy(String domainParticipantQosPolicy) {
        this.domainParticipantQosPolicy = domainParticipantQosPolicy;
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

    public String getSubscriberQoSPolicy() {
        return subscriberQoSPolicy;
    }

    public void setSubscriberQoSPolicy(String subscriberQoSPolicy) {
        this.subscriberQoSPolicy = subscriberQoSPolicy;
    }

    public String getSubscriberTransport() {
        return subscriberTransport;
    }

    public void setSubscriberTransport(String subscriberTransport) {
        this.subscriberTransport = subscriberTransport;
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
