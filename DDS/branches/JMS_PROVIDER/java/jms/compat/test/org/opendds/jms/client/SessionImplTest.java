package org.opendds.jms.client;

import java.util.ArrayList;
import java.util.List;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Destination;
import javax.jms.InvalidDestinationException;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.MessageProducer;
import javax.jms.ObjectMessage;
import javax.jms.Session;
import javax.jms.TemporaryTopic;
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
 * Functional tests for SessionImpl.
 *
 * @author Weiqi Gao
 */

@Remote(endpoint = "http://localhost:8080/opendds-jms-compat/")
@RunWith(RemoteRunner.class)
public class SessionImplTest {
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
    public void createTemporaryTopic() throws JMSException, NamingException {
        final TemporaryTopic topic = session.createTemporaryTopic();
        assert topic != null;
        topic.delete();
    }

    @Test
    public void createConsumerForTemporaryTopic() throws JMSException {
        final TemporaryTopic temporaryTopic = session.createTemporaryTopic();
        assert temporaryTopic != null;

        final MessageConsumer consumer = session.createConsumer(temporaryTopic);
        assert consumer != null;

        consumer.close();
        temporaryTopic.delete();
    }

    @Test(expected = InvalidDestinationException.class)
    public void createConsumerForTemporaryTopicFromAnotherConnection() throws JMSException {
        final TemporaryTopic temporaryTopic = session.createTemporaryTopic();
        assert temporaryTopic != null;

        Session otherSession = otherConnection.createSession(false, Session.AUTO_ACKNOWLEDGE);

        otherSession.createConsumer(temporaryTopic);

        temporaryTopic.delete();
        otherSession.close();
    }

    @Test
    public void createProducersForTemporaryTopic() throws JMSException {
        final TemporaryTopic temporaryTopic = session.createTemporaryTopic();
        assert temporaryTopic != null;

        final MessageProducer producer = session.createProducer(temporaryTopic);
        assert producer != null;

// TODO Make this work again.
//        Session otherSession = otherConnection.createSession(false, Session.AUTO_ACKNOWLEDGE);
//        final MessageProducer producer2 = otherSession.createProducer(temporaryTopic);
//        assert producer2 != null;

        producer.close();
//        producer2.close();
        temporaryTopic.delete();
    }

    @Test
    public void sendMessageToTemporaryTopic() throws JMSException {
        final TemporaryTopic temporaryTopic = session.createTemporaryTopic();

        final MessageProducer producer = session.createProducer(temporaryTopic);
        final MessageConsumer consumer = session.createConsumer(temporaryTopic);

        ObjectMessage objectMessage = session.createObjectMessage();
        assert objectMessage != null;
        objectMessage.setObject(new Integer(1024));

        producer.send(objectMessage);

// TODO Make this work again.
//        final Message messageReceived = consumer.receive();
//        assert messageReceived != null;
//        assert messageReceived instanceof ObjectMessage;
//        ObjectMessage objectMessageReceived = (ObjectMessage) messageReceived;
//        assert (new Integer(1024)).equals(objectMessageReceived.getObject());

        producer.close();
        consumer.close();
        temporaryTopic.delete();
    }

    private void waitFor(int millis) {
        try {
            Thread.sleep(millis); // wait for association
        } catch (InterruptedException e) {
        }
    }

    @Test
    public void replyToScenario() throws JMSException {
        final MessageProducer producer = session.createProducer(destination);
        final MessageConsumer consumer = session.createConsumer(destination);

        // Send original with JMSReplyTo header set to a temporary Topic
        final TextMessage textMessage = session.createTextMessage("Hello");
        final TemporaryTopic topic = session.createTemporaryTopic();
        textMessage.setJMSReplyTo(topic);
        // Set up a consumer to receive replies on temporary topic
        final MessageConsumer replyConsumer = session.createConsumer(topic);
        producer.send(textMessage);

//  TODO Make this work again.
//        // Receive original
//        final Message message = consumer.receive();
//        assert message != null;
//        assert message instanceof TextMessage;
//        final TextMessage textMessage2 = (TextMessage) message;
//        assert "Hello".equals(textMessage2.getText());
//
//        // Getting JMSReplyTo from received message and send reply to it
//        final Destination replyTo = textMessage2.getJMSReplyTo();
//        final MessageProducer replyProducer = session.createProducer(replyTo);
//        textMessage2.clearBody();
//        textMessage2.setText("Goodbye");
//        replyProducer.send(textMessage2);
//
//        // Receive the reploy on the temporary topic
//        final Message message3 = replyConsumer.receive();
//        assert message3 != null;
//        assert message3 instanceof TextMessage;
//        final TextMessage textMessage3 = (TextMessage) message3;
//        assert "Goodbye".equals(textMessage3.getText());

        session.close();
    }

    @Test
    public void recoverSynch() throws JMSException {
        final MessageProducer producer = session.createProducer(destination);
        final MessageConsumer consumer = session.createConsumer(destination);

        waitFor(2500); // wait for association

        final TextMessage textMessage = session.createTextMessage("Hello");
        producer.send(textMessage);
        textMessage.setText("Hello Again");
        producer.send(textMessage);
        textMessage.setText("Goodbye");
        producer.send(textMessage);

// TODO Make this work again
//        final Message message = consumer.receive();
//        assert message != null;
//        final Message message2 = consumer.receive();
//        assert message2 != null;
//        final Message message3 = consumer.receive();
//        assert message3 != null;
//
//        session.recover();
//
//        try {
//            Thread.sleep(1000);
//        } catch (InterruptedException e) {
//            e.printStackTrace();
//        }
//
//        final Message message4 = consumer.receive();
//        assert message4 != null;
//        assert message4.getJMSRedelivered();
//        final Message message5 = consumer.receive();
//        assert message5 != null;
//        assert message5.getJMSRedelivered();
//        final Message message6 = consumer.receive();
//        assert message6 != null;
//        assert message6.getJMSRedelivered();

        session.close();
    }

//    @Test
    public void recoverAsync() throws JMSException {
        Session session = connection.createSession(false, Session.CLIENT_ACKNOWLEDGE);
        final MessageProducer producer = session.createProducer(destination);
        final MessageConsumer consumer = session.createConsumer(destination);
        MyMessageListener myMessageListener = new MyMessageListener();
        consumer.setMessageListener(myMessageListener);

        try {
            Thread.sleep(1000); // Let the listener settle in
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        final TextMessage textMessage = session.createTextMessage("Hello");
        producer.send(textMessage);
        textMessage.setText("Hello Again");
        producer.send(textMessage);
        textMessage.setText("Goodbye");
        producer.send(textMessage);

        try {
            Thread.sleep(5000); // Let message delivery has a chance to complete
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

//  TODO Make this work again.
//        assert 3 == myMessageListener.getOnMessageCount();
//        assert 0 == myMessageListener.getRedeliveryCount();
//        List<String> texts = myMessageListener.getMessageTexts();
//        assert texts.contains("Hello");
//        assert texts.contains("Hello Again");
//        assert texts.contains("Goodbye");
//
//        myMessageListener.reset();
//        session.recover();
//
//        try {
//            Thread.sleep(1000);
//        } catch (InterruptedException e) {
//            e.printStackTrace();
//        }
//
//        assert 3 == myMessageListener.getOnMessageCount();
//        assert 3 == myMessageListener.getRedeliveryCount();
//        texts = myMessageListener.getMessageTexts();
//        assert texts.contains("Hello");
//        assert texts.contains("Hello Again");
//        assert texts.contains("Goodbye");

//        session.close();
    }

//    @Test
    public void close() throws JMSException {
        final MessageProducer producer = session.createProducer(destination);
        final MessageConsumer consumer = session.createConsumer(destination);

        session.close();

        try {
            session.createProducer(destination);
            assert false;
        } catch (Exception e) {
            assert "This Session is closed.".equals(e.getMessage());
        }

        try {
            producer.setPriority(6);
            assert false;
        } catch (Exception e) {
            assert "This MessageProducer is closed.".equals(e.getMessage());
        }

        try {
            consumer.receiveNoWait();
            assert false;
        } catch (Exception e) {
            assert "This MessageConsumer is closed.".equals(e.getMessage());
        }
    }

    private static class MyMessageListener implements MessageListener {
        private int onMessageCount = 0;
        private List<String> messageTexts = new ArrayList<String>();
        private int redeliveryCount = 0;

        public void onMessage(Message message) {
            TextMessage textMessage = (TextMessage) message;
            try {
                String messageText = textMessage.getText();
                messageTexts.add(messageText);
                if (textMessage.getJMSRedelivered()) {
                    redeliveryCount++;
                }
            } catch (JMSException e) {
            }
            onMessageCount++;
        }

        public int getOnMessageCount() {
            return onMessageCount;
        }

        public List<String> getMessageTexts() {
            return messageTexts;
        }

        public int getRedeliveryCount() {
            return redeliveryCount;
        }

        public void reset() {
            onMessageCount = 0;
            messageTexts.clear();
            redeliveryCount = 0;
        }
    }
}
