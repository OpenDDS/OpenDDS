package org.opendds.jms.client;

import java.lang.reflect.Field;
import java.util.Collection;
import java.util.List;
import java.util.Map;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
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
 * @author Weiqi Gao
 */

@Remote(endpoint = "http://localhost:8080/opendds-jms-compat/")
@RunWith(RemoteRunner.class)
public class MessageConsumerImplTest {
    private ConnectionFactory connectionFactory;
    private Destination destination;
    private Connection connection;
    private Connection otherConnection;
    private Session session;

    @Before
    public void setUp() throws NamingException, JMSException {
        InitialContext context = new InitialContext();
        connectionFactory = (ConnectionFactory) context.lookup("DDS/DefaultConnectionFactory");
        destination = (Destination) context.lookup("DDS/DefaultTopic");
        connection = connectionFactory.createConnection();
        otherConnection = connectionFactory.createConnection();
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
        if (otherConnection != null) {
            try {
                otherConnection.close();
            } catch (JMSException e) {
            } finally {
                otherConnection = null;
            }
        }
    }

    @Test
    public void close() throws JMSException {
        final MessageConsumer messageConsumer = session.createConsumer(destination);
        assert messageConsumer != null;
        final Thread thread = new Thread() {
            public void run() {
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    // Don't care
                }
                try {
                    messageConsumer.close();
                } catch (JMSException e) {
                    // Don't care
                }
            }
        };
        thread.start();
        final Message message = messageConsumer.receive();
        assert message == null;
        try {
            thread.join();
        } catch (InterruptedException e) {
            // Don't care
        }
    }

    @Test
    public void receive() throws JMSException {
        final MessageConsumer messageConsumer = session.createConsumer(destination);
        assert messageConsumer != null;

        assert messageConsumer.getMessageSelector() == null;

        // Fake out a MessageProducer and pump some messages
        FakeMessageProducer fakeProducer = new FakeMessageProducer();
        final Thread fakeProducerThread = new Thread(fakeProducer);
        fakeProducerThread.start();

// TODO Make this work again       
//        final Message message = messageConsumer.receive();
//        assert message != null;
//
//        final Message message2 = messageConsumer.receive(100000L);
//        assert message2 != null;
//
//        final Message message3 = messageConsumer.receiveNoWait();
//        assert message3 != null;
//
//        final Message message4 = messageConsumer.receiveNoWait();
//        assert message4 == null;
//
//        messageConsumer.close();
//        try {
//            messageConsumer.receive();
//            assert false;
//        } catch (IllegalStateException e) {
//            assert "This MessageConsumer is closed.".equals(e.getMessage());
//        }
//
//        try {
//            messageConsumer.receiveNoWait();
//            assert false;
//        } catch (IllegalStateException e) {
//            assert "This MessageConsumer is closed.".equals(e.getMessage());
//        }
//
//        try {
//            messageConsumer.receive(100000L);
//            assert false;
//        } catch (IllegalStateException e) {
//            assert "This MessageConsumer is closed.".equals(e.getMessage());
//        }
//
        try {
            fakeProducerThread.join();
            fakeProducer.dispose();
        } catch (InterruptedException e) {
            // Don't care
        }
    }

    @Test
    public void messageListener() throws JMSException {
        final MessageConsumer messageConsumer = session.createConsumer(destination);
        assert messageConsumer != null;

        MyMessageListener messageListener = new MyMessageListener();
        messageConsumer.setMessageListener(messageListener);
        assert messageListener == messageConsumer.getMessageListener();

        messageConsumer.setMessageListener(null);
        assert messageConsumer.getMessageListener() == null;

        messageConsumer.setMessageListener(messageListener);

        // Fake out a MessageProducer and pump some messages
        FakeMessageProducer fakeProducer = new FakeMessageProducer();
        final Thread fakeProducerThread = new Thread(fakeProducer);
        fakeProducerThread.start();

        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            // Don't care
        }
// TODO Make this work again
//        assert messageListener.getOnMessageCallCount() == 3;

        try {
            fakeProducerThread.join();
            fakeProducer.dispose();
        } catch (InterruptedException e) {
            // Don't care
        }
    }

    @Test
    public void acknowledgement() throws JMSException {
        session = connection.createSession(false, Session.CLIENT_ACKNOWLEDGE);
        final MessageConsumer messageConsumer = session.createConsumer(destination);

        // Fake out a MessageProducer and pump some messages
        FakeMessageProducer fakeProducer = new FakeMessageProducer();
        final Thread fakeProducerThread = new Thread(fakeProducer);
        fakeProducerThread.start();

// TODO Make this work again       
//        final Message message = messageConsumer.receive();
//        assert message != null;
//
//        final Message message2 = messageConsumer.receive(100000L);
//        assert message2 != null;
//
//        final Message message3 = messageConsumer.receiveNoWait();
//        assert message3 == null;
//
//        assert getUnacknowledgedCount(session) == 3;
//
//        message.acknowledge();
//
//        assert getUnacknowledgedCount(session) == 0;
//
        try {
            fakeProducerThread.join();
            fakeProducer.dispose();
        } catch (InterruptedException e) {
            // Don't care
        }
    }

    private int getUnacknowledgedCount(Session session) {
        try {
            Class sessionClass = Class.forName("org.opendds.jms.SessionImpl");
            final Field field = sessionClass.getDeclaredField("unacknowledged");
            field.setAccessible(true);
            Map unacknowledged = (Map) field.get(this);
            final Collection<List> listCollection = unacknowledged.values();
            int count = 0;
            for (List pairs : listCollection) {
                count += pairs.size();
            }
            return count;
        } catch (NoSuchFieldException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }
        return Integer.MAX_VALUE; // Unexpected
    }

    private static class MyMessageListener implements MessageListener {

        private int onMessageCallCount = 0;

        public int getOnMessageCallCount() {
            return onMessageCallCount;
        }

        public void setOnMessageCallCount(int onMessageCallCount) {
            this.onMessageCallCount = onMessageCallCount;
        }

        public void onMessage(Message message) {
            onMessageCallCount++;
        }

    }

    private class FakeMessageProducer implements Runnable {

        MessageProducer messageProducer;

        private FakeMessageProducer() throws JMSException {
            messageProducer = session.createProducer(destination);
        }


        public void run() {
            try {
                TextMessage textMessage = session.createTextMessage();

                textMessage.setText("Hello OpenDDS JMS Provider");
                messageProducer.send(textMessage);

                textMessage.setText("Hello again OpenDDS JMS Provider");
                messageProducer.send(textMessage, DeliveryMode.NON_PERSISTENT, 5, 100000L);

                textMessage.setText("Goodbye OpenDDS JMS Provider");
                messageProducer.send(textMessage);
            } catch (JMSException e) {
                // Don't care
            }
        }

        public void dispose() throws JMSException {
            try {
                if (messageProducer != null) {
                    messageProducer.close();
                }
            } catch (JMSException e) {
                // Don't care
            }
        }

    }
}
