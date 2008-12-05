/*
 * $Id$
 */

package org.opendds.jms;

import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
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

/**
 * @author  Weiqi Gao
 * @version $Revision$
 */
// TODO Remove this class once the functional tests in MessageProducerImplTest are running. 
public class TopicMessageProducerImplTest {
    @Test
    public void testDummy() {
        assertTrue(true);
    }

    @Test // Uncomment locally to run this test.  It depends on an externally running DCPSInfoRepo
    public void testSend() throws JMSException {
        // Hack, will use the new Java wrapper to start or stop the DCPSInfoRepo in a few days
        // wqg, Mon Oct 20 12:48:18 CDT 2008
        if (TestUtils.runWithInfoRepo()) {
            doTestSend();
        }
    }

    private void doTestSend() throws JMSException {
        FakeObjects fakeObjects = createFakeObjects();

        Session session = new SessionImpl(null, false, Session.AUTO_ACKNOWLEDGE);
        MessageProducer messageProducer = session.createProducer(fakeObjects.destination);

        assertNotNull(messageProducer);
        messageProducer.send(fakeObjects.message);
        messageProducer.send(fakeObjects.message, DeliveryMode.NON_PERSISTENT, 5, 100000L);

        try {
            messageProducer.send(fakeObjects.destination, fakeObjects.message);
            fail("Should throw");
        } catch (UnsupportedOperationException e) {
            assertEquals("This MessageProducer is created with a Destination.", e.getMessage());
        }

        try {
            messageProducer.send(fakeObjects.destination, fakeObjects.message, DeliveryMode.NON_PERSISTENT, 5, 100000L);
            fail("Should throw");
        } catch (UnsupportedOperationException e) {
            assertEquals("This MessageProducer is created with a Destination.", e.getMessage());
        }

        session.createProducer(null);
        MessageProducer messageProducer2 = session.createProducer(null);
        assertNotNull(messageProducer2);
        messageProducer2.send(fakeObjects.destination, fakeObjects.message);
        messageProducer2.send(fakeObjects.destination, fakeObjects.message, DeliveryMode.NON_PERSISTENT, 5, 100000L);

        try {
            messageProducer2.send(fakeObjects.message);
            fail("Should throw");
        } catch (UnsupportedOperationException e) {
            assertEquals("This MessageProducer is created without a Destination.", e.getMessage());
        }

        try {
            messageProducer2.send(fakeObjects.message, DeliveryMode.NON_PERSISTENT, 5, 100000L);
            fail("Should throw");
        } catch (UnsupportedOperationException e) {
            assertEquals("This MessageProducer is created without a Destination.", e.getMessage());
        }

        messageProducer.setDeliveryMode(DeliveryMode.NON_PERSISTENT);
        assertEquals(DeliveryMode.NON_PERSISTENT, messageProducer.getDeliveryMode());

        messageProducer.setDeliveryMode(DeliveryMode.PERSISTENT);
        assertEquals(DeliveryMode.PERSISTENT, messageProducer.getDeliveryMode());

        try {
            messageProducer.setDeliveryMode(0);
            fail("Should throw");
        } catch (IllegalArgumentException e) {
            assertEquals("Illegal deliveryMode: 0.", e.getMessage());
        }

        try {
            messageProducer.setDeliveryMode(3);
            fail("Should throw");
        } catch (IllegalArgumentException e) {
            assertEquals("Illegal deliveryMode: 3.", e.getMessage());
        }

        messageProducer.setPriority(0);
        assertEquals(0, messageProducer.getPriority());

        messageProducer.setPriority(9);
        assertEquals(9, messageProducer.getPriority());

        try {
            messageProducer.setPriority(-1);
            fail("Should throw");
        } catch (IllegalArgumentException e) {
            assertEquals("Illegal priority: -1.", e.getMessage());
        }

        try {
            messageProducer.setPriority(10);
            fail("Should throw");
        } catch (IllegalArgumentException e) {
            assertEquals("Illegal priority: 10.", e.getMessage());
        }

        messageProducer.setTimeToLive(0L);
        assertEquals(0L, messageProducer.getTimeToLive());

        messageProducer.setTimeToLive(100000L);
        assertEquals(100000L, messageProducer.getTimeToLive());

        try {
            messageProducer.setTimeToLive(-1L);
            fail("Should throw");
        } catch (IllegalArgumentException e) {
            assertEquals("Illegal timeToLive: -1.", e.getMessage());
        }

        assertFalse(messageProducer.getDisableMessageID());
        messageProducer.setDisableMessageID(true); // No-op for now
        assertFalse(messageProducer.getDisableMessageID());

        assertFalse(messageProducer.getDisableMessageTimestamp());
        messageProducer.setDisableMessageTimestamp(true); // No-op for now
        assertFalse(messageProducer.getDisableMessageTimestamp());

        messageProducer.close();
        messageProducer2.close();

        // Just checking
        try {
            messageProducer.send(fakeObjects.message);
            fail("Should throw");
        } catch (JMSException e) {
            assertEquals("This MessageProducer is closed.", e.getMessage());
        }

        try {
            messageProducer2.send(fakeObjects.destination, fakeObjects.message);
            fail("Should throw");
        } catch (JMSException e) {
            assertEquals("This MessageProducer is closed.", e.getMessage());
        }
    }

    private FakeObjects createFakeObjects() throws JMSException {
        FakeObjects fakeObjects = new FakeObjects();
        String[] fakeArgs = new String[]{"-ORBSvcConf", "tcp.conf",
            "-ORBListenEndpoints", "iiop://127.0.0.1:12347",
            "-ORBDebugLevel", "10",
            "-DCPSDebugLevel", "10",
            "-ORBLogFile", "TopicMessageProducerImplTest.log",
            "-DCPSConfigFile", "test.ini"
        };
        DomainParticipantFactory dpFactory = TheParticipantFactory.WithArgs(new StringSeqHolder(fakeArgs));
        assertNotNull(dpFactory);

        DomainParticipant participant = dpFactory.create_participant(1, PARTICIPANT_QOS_DEFAULT.get(), null);
        assertNotNull(participant);

        Publisher publisher = participant.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null);
        assertNotNull(publisher);

        TransportImpl transport = TheTransportFactory.create_transport_impl(1, TheTransportFactory.AUTO_CONFIG);
        assertNotNull(transport);

        AttachStatus attachStatus = transport.attach_to_publisher(publisher);
        assertNotNull(attachStatus);

        Subscriber subscriber = participant.create_subscriber(SUBSCRIBER_QOS_DEFAULT.get(), null);
        assertNotNull(subscriber);

        TransportImpl transport2 = TheTransportFactory.create_transport_impl(2, TheTransportFactory.AUTO_CONFIG);
        assertNotNull(transport2);

        AttachStatus attachStatus2 = transport2.attach_to_subscriber(subscriber);
        assertNotNull(attachStatus2);

        final MessagePayloadTypeSupportImpl typeSupport = new MessagePayloadTypeSupportImpl();
        assertNotNull(typeSupport);

        typeSupport.register_type(participant, "OpenDDS::JMS::MessagePayload");
        final Topic topic = participant.create_topic("OpenDDS::JMS::MessagePayload", typeSupport.get_type_name(), TOPIC_QOS_DEFAULT.get(), null);
        assertNotNull(topic);

        Destination destination = new TopicImpl("Topic1") {
            public Topic createTopic() {
                return topic;
            }
        };

        TextMessage message = new TextMessageImpl(null);
        message.setText("Hello OpenDDS JMS Provider");

        fakeObjects.destination = destination;
        fakeObjects.publisher = publisher;
        fakeObjects.subscriber = subscriber;
        fakeObjects.participant = participant;
        fakeObjects.message = message;

        return fakeObjects;
    }

    private static class FakeObjects {
        public Destination destination;
        public Publisher publisher;
        public Subscriber subscriber;
        public DomainParticipant participant;
        public Message message;
    }
}
