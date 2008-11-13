package org.opendds.jms;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

import javax.jms.Destination;
import javax.jms.InvalidDestinationException;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.ObjectMessage;
import javax.jms.Session;
import javax.jms.TemporaryTopic;
import javax.jms.TextMessage;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import org.junit.Test;
import org.omg.CORBA.StringSeqHolder;

import DDS.DomainParticipant;
import DDS.DomainParticipantFactory;
import DDS.PARTICIPANT_QOS_DEFAULT;
import DDS.PUBLISHER_QOS_DEFAULT;
import DDS.Publisher;
import DDS.SUBSCRIBER_QOS_DEFAULT;
import DDS.Subscriber;
import DDS.TOPIC_QOS_DEFAULT;
import DDS.Topic;
import OpenDDS.DCPS.TheParticipantFactory;
import OpenDDS.DCPS.transport.AttachStatus;
import OpenDDS.DCPS.transport.TheTransportFactory;
import OpenDDS.DCPS.transport.TransportImpl;
import OpenDDS.JMS.MessagePayloadTypeSupportImpl;

public class SessionImplTest {

    @Test
    public void testSessionImpl() throws JMSException {
        // Hack, will use the new Java wrapper to start or stop the DCPSInfoRepo in a few days
        // wqg, Mon Oct 20 12:48:18 CDT 2008
        if (dcpsInfoRepoRunning()) {
            final FakeObjects fakeObjects = createFakeObjects();
            final FakeObjects otherFakeObjects = createOtherFakeObjects();
            // Order is significant
            doTestCreateTemporaryTopic(fakeObjects);
            doTestCreateConsumerForTemporaryTopic(fakeObjects);
            doTestCreateConsumerForTemporaryTopicFromAnotherConnection(fakeObjects, otherFakeObjects);
            doTestCreateProducersForTemporaryTopic(fakeObjects, otherFakeObjects);
            doTestSendingMessageToTemporaryTopic(fakeObjects, otherFakeObjects);
            doTestReplyToScenario(fakeObjects);
        }
    }

    private void doTestCreateTemporaryTopic(FakeObjects fakeObjects) throws JMSException {
        Session session = new SessionImpl(false, Session.AUTO_ACKNOWLEDGE, fakeObjects.participant, fakeObjects.publisher, fakeObjects.subscriber, fakeObjects.connection);
        final TemporaryTopic topic = session.createTemporaryTopic();
        assertNotNull(topic);
        topic.delete();
    }

    private void doTestCreateConsumerForTemporaryTopic(FakeObjects fakeObjects) throws JMSException {
        Session session = new SessionImpl(false, Session.AUTO_ACKNOWLEDGE, fakeObjects.participant, fakeObjects.publisher, fakeObjects.subscriber, fakeObjects.connection);
        final TemporaryTopic temporaryTopic = session.createTemporaryTopic();
        assertNotNull(temporaryTopic);

        final MessageConsumer consumer = session.createConsumer(temporaryTopic);
        assertNotNull(consumer);
        consumer.close();
        temporaryTopic.delete();
    }

    private void doTestCreateConsumerForTemporaryTopicFromAnotherConnection(FakeObjects fakeObjects, FakeObjects otherFakeObjects) throws JMSException {
        Session session = new SessionImpl(false, Session.AUTO_ACKNOWLEDGE, fakeObjects.participant, fakeObjects.publisher, fakeObjects.subscriber, fakeObjects.connection);
        final TemporaryTopic temporaryTopic = session.createTemporaryTopic();
        assertNotNull(temporaryTopic);

        Session otherSession = new SessionImpl(false, Session.AUTO_ACKNOWLEDGE, otherFakeObjects.participant, otherFakeObjects.publisher, otherFakeObjects.subscriber, otherFakeObjects.connection);
        try {
            // JMS 1.1, 4.4.3
            otherSession.createConsumer(temporaryTopic);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof InvalidDestinationException);
        }

        temporaryTopic.delete();
    }

    private void doTestCreateProducersForTemporaryTopic(FakeObjects fakeObjects, FakeObjects otherFakeObjects) throws JMSException {
        Session session = new SessionImpl(false, Session.AUTO_ACKNOWLEDGE, fakeObjects.participant, fakeObjects.publisher, fakeObjects.subscriber, fakeObjects.connection);
        final TemporaryTopic temporaryTopic = session.createTemporaryTopic();
        assertNotNull(temporaryTopic);

        final MessageProducer producer = session.createProducer(temporaryTopic);
        assertNotNull(producer);

        Session otherSession = new SessionImpl(false, Session.AUTO_ACKNOWLEDGE, otherFakeObjects.participant, otherFakeObjects.publisher, otherFakeObjects.subscriber, otherFakeObjects.connection);
        final MessageProducer producer2 = otherSession.createProducer(temporaryTopic);
        assertNotNull(producer2);

        producer.close();
//        producer2.close();
        temporaryTopic.delete();

    }

    private void doTestSendingMessageToTemporaryTopic(FakeObjects fakeObjects, FakeObjects otherFakeObjects) throws JMSException {
        Session session = new SessionImpl(false, Session.AUTO_ACKNOWLEDGE, fakeObjects.participant, fakeObjects.publisher, fakeObjects.subscriber, fakeObjects.connection);
        final TemporaryTopic temporaryTopic = session.createTemporaryTopic();

        final MessageProducer producer = session.createProducer(temporaryTopic);
        final MessageConsumer consumer = session.createConsumer(temporaryTopic);

        ObjectMessage objectMessage = session.createObjectMessage();
        assertNotNull(objectMessage);
        objectMessage.setObject(new Integer(1024));

        producer.send(objectMessage);

        final Message messageReceived = consumer.receive();
        assertNotNull(messageReceived);
        assertTrue(messageReceived instanceof ObjectMessage);
        ObjectMessage objectMessageReceived = (ObjectMessage) messageReceived;
        assertEquals(new Integer(1024), objectMessageReceived.getObject());

        producer.close();
        consumer.close();
        temporaryTopic.delete();
    }


    private void doTestReplyToScenario(FakeObjects fakeObjects) throws JMSException {
        // A JMSReplyTo header scenario
        Session session = new SessionImpl(false, Session.AUTO_ACKNOWLEDGE, fakeObjects.participant, fakeObjects.publisher, fakeObjects.subscriber, fakeObjects.connection);
        final MessageProducer producer = session.createProducer(fakeObjects.destination);
        final MessageConsumer consumer = session.createConsumer(fakeObjects.destination);
        final TemporaryTopic topic = session.createTemporaryTopic();

        // Send original with JMSReplyTo header set to a temporary Topic
        final TextMessage textMessage = session.createTextMessage("Hello");
        textMessage.setJMSReplyTo(topic);
        producer.send(textMessage);

        // Receive original
        final Message message = consumer.receive();
        assertNotNull(message);
        assertTrue(message instanceof TextMessageImpl);
        final TextMessage textMessage2 = (TextMessage) message;
        assertEquals("Hello", textMessage2.getText());

        // Getting JMSReplyTo from received message and send reply to it
        final Destination replyTo = textMessage2.getJMSReplyTo();
        final MessageProducer replyProducer = session.createProducer(replyTo);
        textMessage2.clearBody();
        textMessage2.setText("Goodbye");
        replyProducer.send(textMessage2);

        // Receive the reploy on the temporary topic
        final MessageConsumer replyConsumer = session.createConsumer(topic);
        final Message message3 = replyConsumer.receive();
        assertNotNull(message3);
        assertTrue(message3 instanceof TextMessageImpl);
        final TextMessage textMessage3 = (TextMessage) message3;
        assertEquals("Goodbye", textMessage3.getText());
    }

    private boolean dcpsInfoRepoRunning() {
        // Temporary hack
        try {
            final BufferedReader bufferedReader = new BufferedReader(
                new InputStreamReader(Runtime.getRuntime().exec("netstat -an").getInputStream()));
            String line = null;
            while ((line = bufferedReader.readLine()) != null) {
                if (line.contains(":4096")) return true;
            }
            return false;
        } catch (IOException e) {
            return false;
        }
    }

    private FakeObjects createFakeObjects() {
        FakeObjects fakeObjects = new FakeObjects();

        String[] fakeArgs = new String[]{"-ORBSvcConf", "tcp.conf",
            "-ORBListenEndpoints", "iiop://127.0.0.1:12349",
            "-ORBDebugLevel", "10",
            "-DCPSDebugLevel", "10",
            "-ORBLogFile", "SessionImplTest.log",
            "-DCPSConfigFile", "test.ini"
        };
        DomainParticipantFactory dpFactory = TheParticipantFactory.WithArgs(new StringSeqHolder(fakeArgs));
        assertNotNull(dpFactory);

        DomainParticipant participant = dpFactory.create_participant(1, PARTICIPANT_QOS_DEFAULT.get(), null);
        assertNotNull(participant);

        Subscriber subscriber = participant.create_subscriber(SUBSCRIBER_QOS_DEFAULT.get(), null);
        assertNotNull(subscriber);

        TransportImpl transport = TheTransportFactory.create_transport_impl(1, TheTransportFactory.AUTO_CONFIG);
        assertNotNull(transport);

        AttachStatus attachStatus = transport.attach_to_subscriber(subscriber);
        assertNotNull(attachStatus);

        Publisher publisher = participant.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null);
        assertNotNull(publisher);

        TransportImpl transport2 = TheTransportFactory.create_transport_impl(2, TheTransportFactory.AUTO_CONFIG);
        assertNotNull(transport2);

        AttachStatus attachStatus2 = transport2.attach_to_publisher(publisher);
        assertNotNull(attachStatus2);

        final MessagePayloadTypeSupportImpl typeSupport = new MessagePayloadTypeSupportImpl();
        assertNotNull(typeSupport);

        typeSupport.register_type(participant, "OpenDDS::MessagePayload");
        final Topic topic = participant.create_topic("OpenDDS::MessagePayload", typeSupport.get_type_name(), TOPIC_QOS_DEFAULT.get(), null);
        assertNotNull(topic);

        Destination destination = new TopicImpl("Topic 1") {
            public Topic createTopic() {
                return topic;
            }
        };

        fakeObjects.connection = new ConnectionImpl();
        fakeObjects.participant = participant;
        fakeObjects.publisher = publisher;
        fakeObjects.subscriber = subscriber;
        fakeObjects.destination = destination;

        return fakeObjects;
    }

    private FakeObjects createOtherFakeObjects() {
        FakeObjects fakeObjects = new FakeObjects();

        String[] fakeArgs = new String[]{"-ORBSvcConf", "tcp.conf",
            "-ORBListenEndpoints", "iiop://127.0.0.1:12350",
            "-ORBDebugLevel", "10",
            "-DCPSDebugLevel", "10",
            "-ORBLogFile", "SessionImplTest.log",
            "-DCPSConfigFile", "test.ini"
        };
        DomainParticipantFactory dpFactory = TheParticipantFactory.WithArgs(new StringSeqHolder(fakeArgs));
        assertNotNull(dpFactory);

        DomainParticipant participant = dpFactory.create_participant(2, PARTICIPANT_QOS_DEFAULT.get(), null);
        assertNotNull(participant);

        Subscriber subscriber = participant.create_subscriber(SUBSCRIBER_QOS_DEFAULT.get(), null);
        assertNotNull(subscriber);

        TransportImpl transport = TheTransportFactory.create_transport_impl(3, TheTransportFactory.AUTO_CONFIG);
        assertNotNull(transport);

        AttachStatus attachStatus = transport.attach_to_subscriber(subscriber);
        assertNotNull(attachStatus);

        Publisher publisher = participant.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null);
        assertNotNull(publisher);

        TransportImpl transport2 = TheTransportFactory.create_transport_impl(4, TheTransportFactory.AUTO_CONFIG);
        assertNotNull(transport2);

        AttachStatus attachStatus2 = transport2.attach_to_publisher(publisher);
        assertNotNull(attachStatus2);

        final MessagePayloadTypeSupportImpl typeSupport = new MessagePayloadTypeSupportImpl();
        assertNotNull(typeSupport);

        typeSupport.register_type(participant, "OpenDDS::MessagePayload");
        final Topic topic = participant.create_topic("OpenDDS::MessagePayload", typeSupport.get_type_name(), TOPIC_QOS_DEFAULT.get(), null);
        assertNotNull(topic);

        Destination destination = new TopicImpl("Topic 1") {
            public Topic createTopic() {
                return topic;
            }
        };

        fakeObjects.connection = new ConnectionImpl();
        fakeObjects.participant = participant;
        fakeObjects.publisher = publisher;
        fakeObjects.subscriber = subscriber;
        fakeObjects.destination = destination;

        return fakeObjects;
    }

    private static class FakeObjects {
        public ConnectionImpl connection;
        public ConnectionImpl otherConnection;
        public DomainParticipant participant;
        public Publisher publisher;
        public Subscriber subscriber;
        public Destination destination;
    }
}
