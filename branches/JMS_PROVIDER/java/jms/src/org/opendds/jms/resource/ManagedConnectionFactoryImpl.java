/*
 * $
 */

package org.opendds.jms.resource;

import java.util.Set;
import java.io.PrintWriter;

import javax.resource.spi.ManagedConnectionFactory;
import javax.resource.spi.ConnectionManager;
import javax.resource.spi.ManagedConnection;
import javax.resource.spi.ConnectionRequestInfo;
import javax.resource.ResourceException;
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
        return null;  //To change body of implemented methods use File | Settings | File Templates.
    }

    public Object createConnectionFactory() throws ResourceException {
        return null;  //To change body of implemented methods use File | Settings | File Templates.
    }

    public ManagedConnection createManagedConnection(Subject subject, ConnectionRequestInfo connectionRequestInfo) throws ResourceException {
        return null;  //To change body of implemented methods use File | Settings | File Templates.
    }

    public ManagedConnection matchManagedConnections(Set set, Subject subject, ConnectionRequestInfo connectionRequestInfo) throws ResourceException {
        return null;  //To change body of implemented methods use File | Settings | File Templates.
    }

    public void setLogWriter(PrintWriter printWriter) throws ResourceException {
        //To change body of implemented methods use File | Settings | File Templates.
    }

    public PrintWriter getLogWriter() throws ResourceException {
        return null;  //To change body of implemented methods use File | Settings | File Templates.
    }
}
