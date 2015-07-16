/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
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
 */
public class ConnectionRequestInfoImpl implements ConnectionRequestInfo, Serializable {
    private String clientID;
    private int domainID;
    private ParticipantQosPolicy participantQosPolicy;
    private PublisherQosPolicy publisherQosPolicy;
    private TransportFactory publisherTransport;
    private SubscriberQosPolicy subscriberQosPolicy;
    private TransportFactory subscriberTransport;
    private PersistenceManager persistenceManager;

    public ConnectionRequestInfoImpl(String clientID,
                                     int domainID,
                                     ParticipantQosPolicy participantQosPolicy,
                                     PublisherQosPolicy publisherQosPolicy,
                                     TransportFactory publisherTransport,
                                     SubscriberQosPolicy subscriberQosPolicy,
                                     TransportFactory subscriberTransport,
                                     PersistenceManager persistenceManager) {
        this.clientID = clientID;
        this.domainID = domainID;
        this.participantQosPolicy = participantQosPolicy;
        this.publisherQosPolicy = publisherQosPolicy;
        this.publisherTransport = publisherTransport;
        this.subscriberQosPolicy = subscriberQosPolicy;
        this.subscriberTransport = subscriberTransport;
        this.persistenceManager = persistenceManager;
    }

    public String getClientID() {
        return clientID;
    }

    public int getDomainID() {
        return domainID;
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
            clientID,
            domainID,
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
        return Objects.equals(clientID, cxRequestInfo.clientID)
            && Objects.equals(domainID, cxRequestInfo.domainID)
            && Objects.equals(participantQosPolicy, cxRequestInfo.participantQosPolicy)
            && Objects.equals(publisherQosPolicy, cxRequestInfo.publisherQosPolicy)
            && Objects.equals(publisherTransport, cxRequestInfo.publisherTransport)
            && Objects.equals(subscriberQosPolicy, cxRequestInfo.subscriberQosPolicy)
            && Objects.equals(subscriberTransport, cxRequestInfo.subscriberTransport)
            && Objects.equals(persistenceManager, cxRequestInfo.persistenceManager);
    }
}
