/*
 * $Id$
 */

package org.opendds.jms.resource;

import java.io.Serializable;

import javax.resource.spi.ConnectionRequestInfo;

import OpenDDS.DCPS.transport.TransportConfiguration;

import org.opendds.jms.common.lang.Objects;
import org.opendds.jms.qos.ParticipantQosPolicy;
import org.opendds.jms.qos.PublisherQosPolicy;
import org.opendds.jms.qos.SubscriberQosPolicy;

/**
 * @author Steven Stallion
 * @version $Revision$
 */
public class ConnectionRequestInfoImpl implements ConnectionRequestInfo, Serializable {
    private String clientId;
    private int domainId;
    private ParticipantQosPolicy participantQosPolicy;
    private PublisherQosPolicy publisherQosPolicy;
    private TransportConfiguration publisherTransport;
    private SubscriberQosPolicy subscriberQosPolicy;
    private TransportConfiguration subscriberTransport;

    public ConnectionRequestInfoImpl(String clientId,
                                     int domainId,
                                     ParticipantQosPolicy participantQosPolicy,
                                     PublisherQosPolicy publisherQosPolicy,
                                     TransportConfiguration publisherTransport,
                                     SubscriberQosPolicy subscriberQosPolicy,
                                     TransportConfiguration subscriberTransport) {
        this.clientId = clientId;
        this.domainId = domainId;
        this.participantQosPolicy = participantQosPolicy;
        this.publisherQosPolicy = publisherQosPolicy;
        this.publisherTransport = publisherTransport;
        this.subscriberQosPolicy = subscriberQosPolicy;
        this.subscriberTransport = subscriberTransport;
    }

    public String getClientId() {
        return clientId;
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

    @Override
    public int hashCode() {
        return Objects.hashCode(
            clientId,
            domainId,
            participantQosPolicy,
            publisherQosPolicy,
            publisherTransport,
            subscriberQosPolicy,
            subscriberTransport);
    }

    @Override
    public boolean equals(Object o) {
        if (o == this) {
            return true;
        }

        if (!(o instanceof ConnectionRequestInfoImpl)) {
            return false;
        }

        ConnectionRequestInfoImpl cxRequestInfo = (ConnectionRequestInfoImpl) o;
        return Objects.equals(clientId, cxRequestInfo.clientId) 
            && Objects.equals(domainId, cxRequestInfo.domainId)
            && Objects.equals(participantQosPolicy, cxRequestInfo.participantQosPolicy)
            && Objects.equals(publisherQosPolicy, cxRequestInfo.publisherQosPolicy)
            && Objects.equals(publisherTransport, cxRequestInfo.publisherTransport)
            && Objects.equals(subscriberQosPolicy, cxRequestInfo.subscriberQosPolicy)
            && Objects.equals(subscriberTransport, cxRequestInfo.subscriberTransport);
    }
}
