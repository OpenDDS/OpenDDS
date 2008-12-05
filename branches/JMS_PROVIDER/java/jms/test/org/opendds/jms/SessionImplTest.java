/*
 * $Id$
 */

package org.opendds.jms;

import javax.jms.JMSException;
import javax.resource.ResourceException;

import org.junit.Test;

// TODO Remove this class once the functional tests in SessionImplTest are running.
/**
 * @author  Weiqi Gao
 * @version $Revision$
 */
public class SessionImplTest {

    @Test
    public void testSessionImpl() throws JMSException, ResourceException {
//        // Hack, will use the new Java wrapper to start or stop the DCPSInfoRepo in a few days
//        // wqg, Mon Oct 20 12:48:18 CDT 2008
//        if (TestUtils.runWithInfoRepo()) {
//            final FakeObjects fakeObjects = createFakeObjects();
//            final FakeObjects otherFakeObjects = createOtherFakeObjects();
//            // Order is significant
//            doTestCreateTemporaryTopic(fakeObjects);
//            doTestCreateConsumerForTemporaryTopic(fakeObjects);
//            doTestCreateConsumerForTemporaryTopicFromAnotherConnection(fakeObjects, otherFakeObjects);
//            doTestCreateProducersForTemporaryTopic(fakeObjects, otherFakeObjects);
//            doTestSendingMessageToTemporaryTopic(fakeObjects, otherFakeObjects);
//            doTestReplyToScenario(fakeObjects);
//            doTestRecoverSync(fakeObjects);
//            doTestRecoverAsync(fakeObjects);
//            doTestClose(fakeObjects);
//        }
    }

//    private void doTestCreateTemporaryTopic(FakeObjects fakeObjects) throws JMSException {
//        Session session = new SessionImpl(null, false, Session.AUTO_ACKNOWLEDGE);
//        final TemporaryTopic topic = session.createTemporaryTopic();
//        assertNotNull(topic);
//        topic.delete();
//    }
//
//    private void doTestCreateConsumerForTemporaryTopic(FakeObjects fakeObjects) throws JMSException {
//        Session session = new SessionImpl(null, false, Session.AUTO_ACKNOWLEDGE);
//        final TemporaryTopic temporaryTopic = session.createTemporaryTopic();
//        assertNotNull(temporaryTopic);
//
//        final MessageConsumer consumer = session.createConsumer(temporaryTopic);
//        assertNotNull(consumer);
//        consumer.close();
//        temporaryTopic.delete();
//    }
//
//    private void doTestCreateConsumerForTemporaryTopicFromAnotherConnection(FakeObjects fakeObjects, FakeObjects otherFakeObjects) throws JMSException {
//        Session session = new SessionImpl(null, false, Session.AUTO_ACKNOWLEDGE);
//        final TemporaryTopic temporaryTopic = session.createTemporaryTopic();
//        assertNotNull(temporaryTopic);
//
//        Session otherSession = new SessionImpl(null, false, Session.AUTO_ACKNOWLEDGE);
//        try {
//            // JMS 1.1, 4.4.3
//            otherSession.createConsumer(temporaryTopic);
//            fail("Should throw");
//        } catch (JMSException e) {
//            assertTrue(e instanceof InvalidDestinationException);
//        }
//
//        temporaryTopic.delete();
//    }
//
//    private void doTestCreateProducersForTemporaryTopic(FakeObjects fakeObjects, FakeObjects otherFakeObjects) throws JMSException {
//        Session session = new SessionImpl(null, false, Session.AUTO_ACKNOWLEDGE);
//        final TemporaryTopic temporaryTopic = session.createTemporaryTopic();
//        assertNotNull(temporaryTopic);
//
//        final MessageProducer producer = session.createProducer(temporaryTopic);
//        assertNotNull(producer);
//
//        Session otherSession = new SessionImpl(null, false, Session.AUTO_ACKNOWLEDGE);
//        final MessageProducer producer2 = otherSession.createProducer(temporaryTopic);
//        assertNotNull(producer2);
//
//        producer.close();
////        producer2.close();
//        temporaryTopic.delete();
//    }
//
//    private void doTestSendingMessageToTemporaryTopic(FakeObjects fakeObjects, FakeObjects otherFakeObjects) throws JMSException {
//        Session session = new SessionImpl(null, false, Session.AUTO_ACKNOWLEDGE);
//        final TemporaryTopic temporaryTopic = session.createTemporaryTopic();
//
//        final MessageProducer producer = session.createProducer(temporaryTopic);
//        final MessageConsumer consumer = session.createConsumer(temporaryTopic);
//
//        ObjectMessage objectMessage = session.createObjectMessage();
//        assertNotNull(objectMessage);
//        objectMessage.setObject(new Integer(1024));
//
//        producer.send(objectMessage);
//
//        final Message messageReceived = consumer.receive();
//        assertNotNull(messageReceived);
//        assertTrue(messageReceived instanceof ObjectMessage);
//        ObjectMessage objectMessageReceived = (ObjectMessage) messageReceived;
//        assertEquals(new Integer(1024), objectMessageReceived.getObject());
//
//        producer.close();
//        consumer.close();
//        temporaryTopic.delete();
//    }
//
//    private void doTestReplyToScenario(FakeObjects fakeObjects) throws JMSException {
//        // A JMSReplyTo header scenario
//        Session session = new SessionImpl(null, false, Session.AUTO_ACKNOWLEDGE);
//        final MessageProducer producer = session.createProducer(fakeObjects.destination);
//        final MessageConsumer consumer = session.createConsumer(fakeObjects.destination);
//
//        // Send original with JMSReplyTo header set to a temporary Topic
//        final TextMessage textMessage = session.createTextMessage("Hello");
//        final TemporaryTopic topic = session.createTemporaryTopic();
//        textMessage.setJMSReplyTo(topic);
//        // Set up a consumer to receive replies on temporary topic
//        final MessageConsumer replyConsumer = session.createConsumer(topic);
//        producer.send(textMessage);
//
//        // Receive original
//        final Message message = consumer.receive();
//        assertNotNull(message);
//        assertTrue(message instanceof TextMessageImpl);
//        final TextMessage textMessage2 = (TextMessage) message;
//        assertEquals("Hello", textMessage2.getText());
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
//        assertNotNull(message3);
//        assertTrue(message3 instanceof TextMessageImpl);
//        final TextMessage textMessage3 = (TextMessage) message3;
//        assertEquals("Goodbye", textMessage3.getText());
//
//        session.close();
//    }
//
//    private void doTestRecoverSync(FakeObjects fakeObjects) throws JMSException {
//        Session session = new SessionImpl(null, false, Session.CLIENT_ACKNOWLEDGE);
//        final MessageProducer producer = session.createProducer(fakeObjects.destination);
//        final MessageConsumer consumer = session.createConsumer(fakeObjects.destination);
//
//        final TextMessage textMessage = session.createTextMessage("Hello");
//        producer.send(textMessage);
//        textMessage.setText("Hello Again");
//        producer.send(textMessage);
//        textMessage.setText("Goodbye");
//        producer.send(textMessage);
//
//        try {
//            Thread.sleep(1000);
//        } catch (InterruptedException e) {
//            e.printStackTrace();
//        }
//
//        final Message message = consumer.receive();
//        assertNotNull(message);
//        final Message message2 = consumer.receive();
//        assertNotNull(message);
//        final Message message3 = consumer.receive();
//        assertNotNull(message);
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
//        assertNotNull(message4);
//        assertTrue(message4.getJMSRedelivered());
//        final Message message5 = consumer.receive();
//        assertNotNull(message5);
//        assertTrue(message5.getJMSRedelivered());
//        final Message message6 = consumer.receive();
//        assertNotNull(message6);
//        assertTrue(message6.getJMSRedelivered());
//
//        session.close();
//    }
//
//    private void doTestRecoverAsync(FakeObjects fakeObjects) throws JMSException {
//        Session session = new SessionImpl(null, false, Session.CLIENT_ACKNOWLEDGE);
//        final MessageProducer producer = session.createProducer(fakeObjects.destination);
//        final MessageConsumer consumer = session.createConsumer(fakeObjects.destination);
//        MyMessageListener myMessageListener = new MyMessageListener();
//        consumer.setMessageListener(myMessageListener);
//
//        try {
//            Thread.sleep(1000); // Let the listener settle in
//        } catch (InterruptedException e) {
//            e.printStackTrace();
//        }
//
//        final TextMessage textMessage = session.createTextMessage("Hello");
//        producer.send(textMessage);
//        textMessage.setText("Hello Again");
//        producer.send(textMessage);
//        textMessage.setText("Goodbye");
//        producer.send(textMessage);
//
//        try {
//            Thread.sleep(5000); // Let message delivery has a chance to complete
//        } catch (InterruptedException e) {
//            e.printStackTrace();
//        }
//
//        assertEquals(3, myMessageListener.getOnMessageCount());
//        assertEquals(0, myMessageListener.getRedeliveryCount());
//        List<String> texts = myMessageListener.getMessageTexts();
//        assertTrue(texts.contains("Hello"));
//        assertTrue(texts.contains("Hello Again"));
//        assertTrue(texts.contains("Goodbye"));
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
//        assertEquals(3, myMessageListener.getOnMessageCount());
//        assertEquals(3, myMessageListener.getRedeliveryCount());
//        texts = myMessageListener.getMessageTexts();
//        assertTrue(texts.contains("Hello"));
//        assertTrue(texts.contains("Hello Again"));
//        assertTrue(texts.contains("Goodbye"));
//
//        session.close();
//    }
//
//    private void doTestClose(FakeObjects fakeObjects) throws JMSException {
//        Session session = new SessionImpl(null, false, Session.AUTO_ACKNOWLEDGE);
//        final MessageProducer producer = session.createProducer(fakeObjects.destination);
//        final MessageConsumer consumer = session.createConsumer(fakeObjects.destination);
//
//        session.close();
//
//        try {
//            session.createProducer(fakeObjects.destination);
//            fail("Should throw");
//        } catch (Exception e) {
//            assertEquals("This Session is closed.", e.getMessage());
//        }
//
//        try {
//            producer.setPriority(6);
//            fail("Should throw");
//        } catch (Exception e) {
//            assertEquals("This MessageProducer is closed.", e.getMessage());
//        }
//
//        try {
//            consumer.receiveNoWait();
//            fail("Should throw");
//        } catch (Exception e) {
//            assertEquals("This MessageConsumer is closed.", e.getMessage());
//        }
//    }
//
//    private FakeObjects createFakeObjects() throws ResourceException {
//        FakeObjects fakeObjects = new FakeObjects();
//
//        String[] fakeArgs = new String[]{"-ORBSvcConf", "tcp.conf",
//            "-ORBListenEndpoints", "iiop://127.0.0.1:12349",
//            "-ORBDebugLevel", "10",
//            "-DCPSDebugLevel", "10",
//            "-ORBLogFile", "SessionImplTest.log",
//            "-DCPSConfigFile", "test.ini"
//        };
//        DomainParticipantFactory dpFactory = TheParticipantFactory.WithArgs(new StringSeqHolder(fakeArgs));
//        assertNotNull(dpFactory);
//
//        DomainParticipant participant = dpFactory.create_participant(1, PARTICIPANT_QOS_DEFAULT.get(), null);
//        assertNotNull(participant);
//
//        Subscriber subscriber = participant.create_subscriber(SUBSCRIBER_QOS_DEFAULT.get(), null);
//        assertNotNull(subscriber);
//
//        TransportImpl transport = TheTransportFactory.create_transport_impl(1, TheTransportFactory.AUTO_CONFIG);
//        assertNotNull(transport);
//
//        AttachStatus attachStatus = transport.attach_to_subscriber(subscriber);
//        assertNotNull(attachStatus);
//
//        Publisher publisher = participant.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null);
//        assertNotNull(publisher);
//
//        TransportImpl transport2 = TheTransportFactory.create_transport_impl(2, TheTransportFactory.AUTO_CONFIG);
//        assertNotNull(transport2);
//
//        AttachStatus attachStatus2 = transport2.attach_to_publisher(publisher);
//        assertNotNull(attachStatus2);
//
//        final MessagePayloadTypeSupportImpl typeSupport = new MessagePayloadTypeSupportImpl();
//        assertNotNull(typeSupport);
//
//        typeSupport.register_type(participant, "OpenDDS::JMS::MessagePayload");
//        final Topic topic = participant.create_topic("OpenDDS::JMS::MessagePayload", typeSupport.get_type_name(), TOPIC_QOS_DEFAULT.get(), null);
//        assertNotNull(topic);
//
//        Destination destination = new TopicImpl("Topic 1") {
//            public Topic createTopic() {
//                return topic;
//            }
//        };
//
//        fakeObjects.connection = new ConnectionImpl(new ManagedConnectionImpl(new Subject(), new ConnectionRequestInfoImpl("clientID", 2, null, null, null, null, null, null)));
//
//        fakeObjects.participant = participant;
//        fakeObjects.publisher = publisher;
//        fakeObjects.subscriber = subscriber;
//        fakeObjects.destination = destination;
//
//        return fakeObjects;
//    }
//
//    private FakeObjects createOtherFakeObjects() throws ResourceException {
//        FakeObjects fakeObjects = new FakeObjects();
//
//        String[] fakeArgs = new String[]{"-ORBSvcConf", "tcp.conf",
//            "-ORBListenEndpoints", "iiop://127.0.0.1:12350",
//            "-ORBDebugLevel", "10",
//            "-DCPSDebugLevel", "10",
//            "-ORBLogFile", "SessionImplTest.log",
//            "-DCPSConfigFile", "test.ini"
//        };
//        DomainParticipantFactory dpFactory = TheParticipantFactory.WithArgs(new StringSeqHolder(fakeArgs));
//        assertNotNull(dpFactory);
//
//        DomainParticipant participant = dpFactory.create_participant(2, PARTICIPANT_QOS_DEFAULT.get(), null);
//        assertNotNull(participant);
//
//        Subscriber subscriber = participant.create_subscriber(SUBSCRIBER_QOS_DEFAULT.get(), null);
//        assertNotNull(subscriber);
//
//        TransportImpl transport = TheTransportFactory.create_transport_impl(3, TheTransportFactory.AUTO_CONFIG);
//        assertNotNull(transport);
//
//        AttachStatus attachStatus = transport.attach_to_subscriber(subscriber);
//        assertNotNull(attachStatus);
//
//        Publisher publisher = participant.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null);
//        assertNotNull(publisher);
//
//        TransportImpl transport2 = TheTransportFactory.create_transport_impl(4, TheTransportFactory.AUTO_CONFIG);
//        assertNotNull(transport2);
//
//        AttachStatus attachStatus2 = transport2.attach_to_publisher(publisher);
//        assertNotNull(attachStatus2);
//
//        final MessagePayloadTypeSupportImpl typeSupport = new MessagePayloadTypeSupportImpl();
//        assertNotNull(typeSupport);
//
//        typeSupport.register_type(participant, "OpenDDS::JMS::MessagePayload");
//        final Topic topic = participant.create_topic("OpenDDS::JMS::MessagePayload", typeSupport.get_type_name(), TOPIC_QOS_DEFAULT.get(), null);
//        assertNotNull(topic);
//
//        Destination destination = new TopicImpl("Topic 1") {
//            public Topic createTopic() {
//                return topic;
//            }
//        };
//
//        fakeObjects.connection = new ConnectionImpl(new ManagedConnectionImpl(new Subject(), new ConnectionRequestInfoImpl("clientID", 2, null, null, null, null, null, null)));
//        fakeObjects.participant = participant;
//        fakeObjects.publisher = publisher;
//        fakeObjects.subscriber = subscriber;
//        fakeObjects.destination = destination;
//
//        return fakeObjects;
//    }
//
//    private static class FakeObjects {
//        public ConnectionImpl connection;
//        public ConnectionImpl otherConnection;
//        public DomainParticipant participant;
//        public Publisher publisher;
//        public Subscriber subscriber;
//        public Destination destination;
//    }
//
//    private class MyMessageListener implements MessageListener {
//        private int onMessageCount = 0;
//        private List<String> messageTexts = new ArrayList<String>();
//        private int redeliveryCount = 0;
//
//        public void onMessage(Message message) {
//            TextMessage textMessage = (TextMessage) message;
//            try {
//                String messageText = textMessage.getText();
//                messageTexts.add(messageText);
//                if (textMessage.getJMSRedelivered()) {
//                    redeliveryCount++;
//                }
//            } catch (JMSException e) {
//            }
//            onMessageCount++;
//        }
//
//        public int getOnMessageCount() {
//            return onMessageCount;
//        }
//
//        public List<String> getMessageTexts() {
//            return messageTexts;
//        }
//
//        public int getRedeliveryCount() {
//            return redeliveryCount;
//        }
//
//        public void reset() {
//            onMessageCount = 0;
//            messageTexts.clear();
//            redeliveryCount = 0;
//        }
//    }
}
