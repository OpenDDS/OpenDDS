/*
 * $Id$
 */

package org.opendds.jms;

import java.util.Properties;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ConnectionFactoryImpl implements ConnectionFactory {
    private Properties participantQosPolicy;
    private Properties publisherQosPolicy;
    private Properties subscriberQosPolicy;
    private Properties transportConfiguration;

    public Properties getParticipantQosPolicy() {
        return participantQosPolicy;
    }

    public void setParticipantQosPolicy(Properties participantQosPolicy) {
        this.participantQosPolicy = participantQosPolicy;
    }

    public Properties getPublisherQosPolicy() {
        return publisherQosPolicy;
    }

    public void setPublisherQosPolicy(Properties publisherQosPolicy) {
        this.publisherQosPolicy = publisherQosPolicy;
    }

    public Properties getSubscriberQosPolicy() {
        return subscriberQosPolicy;
    }

    public void setSubscriberQosPolicy(Properties subscriberQosPolicy) {
        this.subscriberQosPolicy = subscriberQosPolicy;
    }

    public Properties getTransportConfiguration() {
        return transportConfiguration;
    }

    public void setTransportConfiguration(Properties transportConfiguration) {
        this.transportConfiguration = transportConfiguration;
    }

    public Connection createConnection() throws JMSException {
        return null;
    }

    public Connection createConnection(String userName, String password) {
        throw new UnsupportedOperationException();
    }
}
