/*
 * $Id$
 */

package org.opendds.jms.resource;

import java.io.Serializable;

import javax.resource.spi.ConnectionRequestInfo;

import org.opendds.jms.common.lang.Objects;
import org.opendds.jms.persistence.PersistenceManager;
import org.opendds.jms.qos.ParticipantQosPolicy;
import org.opendds.jms.qos.PublisherQosPolicy;
import org.opendds.jms.qos.SubscriberQosPolicy;
import org.opendds.jms.transport.TransportFactory;

/**
 * @author Steven Stallion
 * @version $Revision$
 */
public class ConnectionRequestInfoImpl implements ConnectionRequestInfo, Serializable {
    private String clientId;
    private int domainId;
    private ParticipantQosPolicy participantQosPolicy;
    private PublisherQosPolicy publisherQosPolicy;
    private TransportFactory publisherTransport;
    private SubscriberQosPolicy subscriberQosPolicy;
    private TransportFactory subscriberTransport;
    private PersistenceManager persistenceManager;

    public ConnectionRequestInfoImpl(String clientId,
                                     int domainId,
                                     ParticipantQosPolicy participantQosPolicy,
                                     PublisherQosPolicy publisherQosPolicy,
                                     TransportFactory publisherTransport,
                                     SubscriberQosPolicy subscriberQosPolicy,
                                     TransportFactory subscriberTransport,
                                     PersistenceManager persistenceManager) {
        this.clientId = clientId;
        this.domainId = domainId;
        this.participantQosPolicy = participantQosPolicy;
        this.publisherQosPolicy = publisherQosPolicy;
        this.publisherTransport = publisherTransport;
        this.subscriberQosPolicy = subscriberQosPolicy;
        this.subscriberTransport = subscriberTransport;
        this.persistenceManager = persistenceManager;
    }

    public String getClientID() {
        return clientId;
    }

    public int getDomainID() {
        return domainId;
    }

    public ParticipantQosPolicy getParticipantQosPolicy() {
        return participantQosPolicy;
    }

    public PublisherQosPolicy getPublisherQosPolicy() {
        return publisherQosPolicy;
    }

    public TransportFactory getPublisherTransport() {
        return publisherTransport;
    }

    public SubscriberQosPolicy getSubscriberQosPolicy() {
        return subscriberQosPolicy;
    }

    public TransportFactory getSubscriberTransport() {
        return subscriberTransport;
    }

    public PersistenceManager getPersistenceManager() {
        return persistenceManager;
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
            persistenceManager);
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
            && Objects.equals(subscriberTransport, cxRequestInfo.subscriberTransport)
            && Objects.equals(persistenceManager, cxRequestInfo.persistenceManager);
    }
}
