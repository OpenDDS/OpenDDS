/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.naming.InitialContext;
import javax.naming.NamingException;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.remote.annotation.Remote;
import org.junit.remote.runner.RemoteRunner;
import org.junit.runner.RunWith;

/**
 * @author  Weiqi Gao
 */
@Remote(endpoint = "http://localhost:8080/opendds-jms-compat/")
@RunWith(RemoteRunner.class)
public class MessageProducerImplTest {
    private ConnectionFactory connectionFactory;
    private Destination destination;
    private Connection connection;
    private Session session;

    @Before
    public void setUp() throws NamingException, JMSException {
        InitialContext context = new InitialContext();
        connectionFactory = (ConnectionFactory) context.lookup("DDS/DefaultConnectionFactory");
        destination = (Destination) context.lookup("DDS/DefaultTopic");
        connection = connectionFactory.createConnection();
        session = connection.createSession(false, Session.AUTO_ACKNOWLEDGE);
    }

    @After
    public void tearDown() {
        if (session != null) {
            try {
                session.close();
            } catch (JMSException e) {
            } finally {
                session = null;
            }
        }
        if (connection != null) {
            try {
                connection.close();
            } catch (JMSException e) {
            } finally {
                connection = null;
            }
        }
    }

    @Test
    public void sendThroughProducerWithDestination() throws JMSException {
        MessageProducer messageProducer = session.createProducer(destination);
        assert messageProducer != null;

        TextMessage message = session.createTextMessage();

        messageProducer.send(message);
        messageProducer.send(message, DeliveryMode.NON_PERSISTENT, 5, 100000L);

        try {
            messageProducer.send(destination, message);
            assert false;
        } catch (UnsupportedOperationException e) {
            assert "This MessageProducer is created with a Destination.".equals(e.getMessage());
        }

        try {
            messageProducer.send(destination, message, DeliveryMode.NON_PERSISTENT, 5, 100000L);
            assert false;
        } catch (UnsupportedOperationException e) {
            assert "This MessageProducer is created with a Destination.".equals(e.getMessage());
        }

        messageProducer.close();
    }

    @Test
    public void sendThroughProducerWithoutDestination() throws JMSException {
        MessageProducer messageProducer = session.createProducer(null);
        assert messageProducer != null;

        TextMessage message = session.createTextMessage();

        messageProducer.send(destination, message);
        messageProducer.send(destination, message, DeliveryMode.NON_PERSISTENT, 5, 100000L);

        try {
            messageProducer.send(message);
            assert false;
        } catch (UnsupportedOperationException e) {
            assert "This MessageProducer is created without a Destination.".equals(e.getMessage());
        }

        try {
            messageProducer.send(message, DeliveryMode.NON_PERSISTENT, 5, 100000L);
            assert false;
        } catch (UnsupportedOperationException e) {
            assert "This MessageProducer is created without a Destination.".equals(e.getMessage());
        }

        messageProducer.close();
    }

    @Test
    public void deliveryMode() throws JMSException {
        MessageProducer messageProducer = session.createProducer(destination);
        assert messageProducer != null;

        messageProducer.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
        assert DeliveryMode.NON_PERSISTENT == messageProducer.getDeliveryMode();

        messageProducer.setDeliveryMode(DeliveryMode.PERSISTENT);
        assert DeliveryMode.PERSISTENT == messageProducer.getDeliveryMode();

        try {
            messageProducer.setDeliveryMode(0);
            assert false;
        } catch (IllegalArgumentException e) {
            assert "Illegal deliveryMode: 0.".equals(e.getMessage());
        }

        try {
            messageProducer.setDeliveryMode(3);
            assert false;
        } catch (IllegalArgumentException e) {
            assert "Illegal deliveryMode: 3.".equals(e.getMessage());
        }

        messageProducer.close();
    }

    @Test
    public void priority() throws JMSException {
        MessageProducer messageProducer = session.createProducer(destination);
        assert messageProducer != null;

        messageProducer.setPriority(0);
        assert 0 == messageProducer.getPriority();

        messageProducer.setPriority(9);
        assert 9 == messageProducer.getPriority();

        try {
            messageProducer.setPriority(-1);
            assert false;
        } catch (IllegalArgumentException e) {
            assert "Illegal priority: -1.".equals(e.getMessage());
        }

        try {
            messageProducer.setPriority(10);
            assert false;
        } catch (IllegalArgumentException e) {
            assert "Illegal priority: 10.".equals(e.getMessage());
        }
        messageProducer.close();
    }

    @Test
    public void timeToLive() throws JMSException {
        MessageProducer messageProducer = session.createProducer(destination);
        assert messageProducer != null;

        messageProducer.setTimeToLive(0L);
        assert 0L == messageProducer.getTimeToLive();

        messageProducer.setTimeToLive(100000L);
        assert 100000L == messageProducer.getTimeToLive();

        try {
            messageProducer.setTimeToLive(-1L);
            assert false;
        } catch (IllegalArgumentException e) {
            assert "Illegal timeToLive: -1.".equals(e.getMessage());
        }
        messageProducer.close();
    }

    @Test
    public void messageID() throws JMSException {
        MessageProducer messageProducer = session.createProducer(destination);
        assert messageProducer != null;

        assert !messageProducer.getDisableMessageID();
        messageProducer.setDisableMessageID(true); // No-op for now
        assert !messageProducer.getDisableMessageID();

        messageProducer.close();
    }

    @Test
    public void messageTimestamp() throws JMSException {
        MessageProducer messageProducer = session.createProducer(destination);
        assert messageProducer != null;

        assert !messageProducer.getDisableMessageTimestamp();
        messageProducer.setDisableMessageTimestamp(true); // No-op for now
        assert !messageProducer.getDisableMessageTimestamp();

        messageProducer.close();
    }

    @Test
    public void close() throws JMSException {
        MessageProducer messageProducer = session.createProducer(destination);
        assert messageProducer != null;

        TextMessage message = session.createTextMessage();

        messageProducer.close();

        try {
            messageProducer.send(message);
            assert false;
        } catch (JMSException e) {
            assert "This MessageProducer is closed.".equals(e.getMessage());
        }
    }
}
