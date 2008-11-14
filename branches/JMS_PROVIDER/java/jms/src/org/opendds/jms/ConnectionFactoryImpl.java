/*
 * $Id$
 */

package org.opendds.jms;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;

import OpenDDS.DCPS.transport.TransportConfiguration;

import org.opendds.jms.qos.ParticipantQosPolicy;
import org.opendds.jms.qos.PublisherQosPolicy;
import org.opendds.jms.qos.SubscriberQosPolicy;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ConnectionFactoryImpl implements ConnectionFactory {
    private int domainId;
    private ParticipantQosPolicy participantQosPolicy;
    private PublisherQosPolicy publisherQosPolicy;
    private TransportConfiguration publisherTransport;
    private SubscriberQosPolicy subscriberQosPolicy;
    private TransportConfiguration subscriberTransport;

    public ConnectionFactoryImpl(int domainId,
                                 ParticipantQosPolicy participantQosPolicy,
                                 PublisherQosPolicy publisherQosPolicy,
                                 TransportConfiguration publisherTransport,
                                 SubscriberQosPolicy subscriberQosPolicy,
                                 TransportConfiguration subscriberTransport) {
        this.domainId = domainId;
        this.participantQosPolicy = participantQosPolicy;
        this.publisherQosPolicy = publisherQosPolicy;
        this.publisherTransport = publisherTransport;
        this.subscriberQosPolicy = subscriberQosPolicy;
        this.subscriberTransport = subscriberTransport;
    }

    public int getDomainId() {
        return domainId;
    }

    public ParticipantQosPolicy getParticipantQosPolicy() {
        return participantQosPolicy;
    }

    public PublisherQosPolicy getPublisherQosPolicy() {
        return publisherQosPolicy;
    }

    public TransportConfiguration getPublisherTransport() {
        return publisherTransport;
    }

    public SubscriberQosPolicy getSubscriberQosPolicy() {
        return subscriberQosPolicy;
    }

    public TransportConfiguration getSubscriberTransport() {
        return subscriberTransport;
    }

    public Connection createConnection() throws JMSException {
        return null;
    }

    public Connection createConnection(String userName, String password) {
        throw new UnsupportedOperationException();
    }
}
