/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.resource;

import java.io.PrintWriter;
import java.util.Iterator;
import java.util.Set;

import javax.naming.NamingException;
import javax.resource.ResourceException;
import javax.resource.spi.ConnectionManager;
import javax.resource.spi.ConnectionRequestInfo;
import javax.resource.spi.IllegalStateException;
import javax.resource.spi.ManagedConnection;
import javax.resource.spi.ManagedConnectionFactory;
import javax.security.auth.Subject;

import org.opendds.jms.ConnectionFactoryImpl;
import org.opendds.jms.common.lang.Objects;
import org.opendds.jms.common.lang.Strings;
import org.opendds.jms.common.util.JndiHelper;
import org.opendds.jms.common.util.Logger;
import org.opendds.jms.persistence.PersistenceManager;
import org.opendds.jms.qos.ParticipantQosPolicy;
import org.opendds.jms.qos.PublisherQosPolicy;
import org.opendds.jms.qos.SubscriberQosPolicy;
import org.opendds.jms.transport.TransportFactory;

/**
 * @author  Steven Stallion
 */
public class ManagedConnectionFactoryImpl implements ManagedConnectionFactory {
    private static Logger logger = Logger.getLogger(ManagedConnectionFactoryImpl.class);

    private String clientID;
    private Integer domainID;
    private String participantQosPolicy;
    private String publisherQosPolicy;
    private String publisherTransport;
    private String subscriberQosPolicy;
    private String subscriberTransport;
    private String transportType;
    private String persistenceManager;

    public String getClientID() {
        return clientID;
    }

    public void setClientID(String clientID) {
        this.clientID = clientID;
    }

    public Integer getDomainID() {
        return domainID;
    }

    public void setDomainID(Integer domainID) {
        this.domainID = domainID;
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

    public String getSubscriberTransport() {
        return subscriberTransport;
    }

    public void setSubscriberTransport(String subscriberTransport) {
        this.subscriberTransport = subscriberTransport;
    }

    public String getTransportType() {
        return transportType;
    }

    public void setTransportType(String transportType) {
        this.transportType = transportType;
    }

    public String getPersistenceManager() {
        return persistenceManager;
    }

    public void setPersistenceManager(String persistenceManager) {
        this.persistenceManager = persistenceManager;
    }

    public PrintWriter getLogWriter() {
        return null; // logging disabled
    }

    public void setLogWriter(PrintWriter log) {}

    public void validate() throws ResourceException {
        if (domainID == null) {
            throw new IllegalStateException("DomainId is a required config-property!");
        }

        if (Strings.isEmpty(transportType)) {
            throw new IllegalStateException("TransportType is a required config-property!");
        }
    }

    public Object createConnectionFactory() throws ResourceException {
        return createConnectionFactory(null);
    }

    public Object createConnectionFactory(ConnectionManager cxManager) throws ResourceException {
        validate();

        PersistenceManager persistenceManager = null;
        if (!Strings.isEmpty(this.persistenceManager)) {
            JndiHelper helper = new JndiHelper();
            try {
                persistenceManager = helper.lookup(this.persistenceManager);

            } catch (NamingException e) {
                throw new ResourceException(e);
            }
        }

        ConnectionRequestInfo cxRequestInfo =
            new ConnectionRequestInfoImpl(clientID, domainID,
                new ParticipantQosPolicy(participantQosPolicy),
                new PublisherQosPolicy(publisherQosPolicy),
                new TransportFactory(transportType, publisherTransport),
                new SubscriberQosPolicy(subscriberQosPolicy),
                new TransportFactory(transportType, subscriberTransport),
                persistenceManager);

        Object mcf = new ConnectionFactoryImpl(this, cxManager, cxRequestInfo);
        logger.debug("Created %s", mcf);

        return mcf;
    }

    public ManagedConnection createManagedConnection(Subject subject,
                                                     ConnectionRequestInfo cxRequestInfo) throws ResourceException {

        if (!(cxRequestInfo instanceof ConnectionRequestInfoImpl)) {
            throw new IllegalArgumentException();
        }

        ManagedConnection mc = new ManagedConnectionImpl(subject, (ConnectionRequestInfoImpl) cxRequestInfo);
        logger.debug("Created %s", mc);

        return mc;
    }

    public ManagedConnection matchManagedConnections(Set connectionSet,
                                                     Subject subject,
                                                     ConnectionRequestInfo cxRequestInfo) throws ResourceException {
        Iterator itr = connectionSet.iterator();
        while (itr.hasNext()) {
            Object o = itr.next();
            if (o instanceof ManagedConnectionImpl) {
                ManagedConnectionImpl connection = (ManagedConnectionImpl) o;
                if (connection.matches(subject, cxRequestInfo)) {
                    return connection;
                }
            }
        }
        return null;
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
            transportType,
            persistenceManager);
    }

    @Override
    public boolean equals(Object o) {
        if (o == this) {
            return true;
        }

        if (!(o instanceof ManagedConnectionFactoryImpl)) {
            return false;
        }

        ManagedConnectionFactoryImpl mcf = (ManagedConnectionFactoryImpl) o;
        return Objects.equals(clientID, mcf.clientID)
            && Objects.equals(domainID, mcf.domainID)
            && Objects.equals(participantQosPolicy, mcf.participantQosPolicy)
            && Objects.equals(publisherQosPolicy, mcf.publisherQosPolicy)
            && Objects.equals(publisherTransport, mcf.publisherTransport)
            && Objects.equals(subscriberQosPolicy, mcf.subscriberQosPolicy)
            && Objects.equals(subscriberTransport, mcf.subscriberTransport)
            && Objects.equals(transportType, mcf.transportType)
            && Objects.equals(persistenceManager, mcf.persistenceManager);
    }
}
