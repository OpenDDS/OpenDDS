/*
 * $Id$
 */

package org.opendds.jms;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.resource.spi.ConnectionManager;

import OpenDDS.DCPS.transport.TransportConfiguration;

import org.opendds.jms.qos.ParticipantQosPolicy;
import org.opendds.jms.qos.PublisherQosPolicy;
import org.opendds.jms.qos.SubscriberQosPolicy;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ConnectionFactoryImpl implements ConnectionFactory {
    private ConnectionManager manager;
    private int domainId;
    private ParticipantQosPolicy participantQosPolicy;
    private PublisherQosPolicy publisherQosPolicy;
    private TransportConfiguration publisherTransport;
    private SubscriberQosPolicy subscriberQosPolicy;
    private TransportConfiguration subscriberTransport;

    public ConnectionFactoryImpl(ConnectionManager manager) {
        this.manager = manager;
    }

    public int getDomainId() {
        return domainId;
    }

    public void setDomainId(int domainId) {
        this.domainId = domainId;
    }

    public ParticipantQosPolicy getParticipantQosPolicy() {
        return participantQosPolicy;
    }

    public void setParticipantQosPolicy(ParticipantQosPolicy participantQosPolicy) {
        this.participantQosPolicy = participantQosPolicy;
    }

    public PublisherQosPolicy getPublisherQosPolicy() {
        return publisherQosPolicy;
    }

    public void setPublisherQosPolicy(PublisherQosPolicy publisherQosPolicy) {
        this.publisherQosPolicy = publisherQosPolicy;
    }

    public TransportConfiguration getPublisherTransport() {
        return publisherTransport;
    }

    public void setPublisherTransport(TransportConfiguration publisherTransport) {
        this.publisherTransport = publisherTransport;
    }

    public SubscriberQosPolicy getSubscriberQosPolicy() {
        return subscriberQosPolicy;
    }

    public void setSubscriberQosPolicy(SubscriberQosPolicy subscriberQosPolicy) {
        this.subscriberQosPolicy = subscriberQosPolicy;
    }

    public TransportConfiguration getSubscriberTransport() {
        return subscriberTransport;
    }

    public void setSubscriberTransport(TransportConfiguration subscriberTransport) {
        this.subscriberTransport = subscriberTransport;
    }

    public Connection createConnection() throws JMSException {
        return null;
    }

    public Connection createConnection(String userName, String password) {
        throw new UnsupportedOperationException();
    }
}
