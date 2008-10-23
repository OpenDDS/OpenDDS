package org.opendds.jms;

import DDS.DomainParticipant;
import DDS.DomainParticipantFactory;
import DDS.PARTICIPANT_QOS_DEFAULT;
import DDS.PUBLISHER_QOS_DEFAULT;
import DDS.Publisher;
import DDS.TOPIC_QOS_DEFAULT;
import DDS.Topic;
import OpenDDS.DCPS.TheParticipantFactory;
import OpenDDS.DCPS.transport.AttachStatus;
import OpenDDS.DCPS.transport.TheTransportFactory;
import OpenDDS.DCPS.transport.TransportImpl;
import OpenDDS.JMS.MessagePayloadTypeSupportImpl;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageProducer;
import javax.jms.TextMessage;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import org.junit.Test;
import org.omg.CORBA.StringSeqHolder;

public class TopicMessageProducerImplTest {
    @Test
    public void testDummy() {
        assertTrue(true);
    }

    // @Test Uncomment locally to run this test.  It depends on an externally running DCPSInfoRepo
    public void testSend() throws JMSException {
        // Hack, will use the new Java wrapper to start or stop the DCPSInfoRepo in a few days
        // wqg, Mon Oct 20 12:48:18 CDT 2008
        if (dcpsInfoRepoRunning()) {
            doTestSend();
        }
    }

    private void doTestSend() throws JMSException {
        FakeObjects fakeObjects = createFakeObjects();

        MessageProducer messageProducer = new TopicMessageProducerImpl(fakeObjects.destination,
            fakeObjects.publisher, fakeObjects.participant);
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

        MessageProducer messageProducer2 = new TopicMessageProducerImpl(null,
            fakeObjects.publisher, fakeObjects.participant);
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
            "-ORBLogFile", "test.log",
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

        final MessagePayloadTypeSupportImpl typeSupport = new MessagePayloadTypeSupportImpl();
        assertNotNull(typeSupport);

        typeSupport.register_type(participant, "OpenDDS::MessagePayload");
        final Topic topic = participant.create_topic("OpenDDS::MessagePayload", typeSupport.get_type_name(), TOPIC_QOS_DEFAULT.get(), null);
        assertNotNull(topic);

        Destination destination = new TopicImpl() {
            public Topic extractDDSTopic() {
                return topic;
            }
        };

        TextMessage message = new TextMessageImpl();
        message.setText("Hello OpenDDS JMS Provider");

        fakeObjects.destination = destination;
        fakeObjects.publisher = publisher;
        fakeObjects.participant = participant;
        fakeObjects.message = message;

        return fakeObjects;
    }


    private boolean dcpsInfoRepoRunning() {
        // Temporary hack
        try {
            final BufferedReader bufferedReader = new BufferedReader(
                new InputStreamReader(Runtime.getRuntime().exec("netstat -an").getInputStream()));
            String line = null;
            while ((line = bufferedReader.readLine()) != null) {
                if (line.contains(":1111")) return true;
            }
            return false;
        } catch (IOException e) {
            return false;
        }
    }

    private static class FakeObjects {
        public Destination destination;
        public Publisher publisher;
        public DomainParticipant participant;
        public Message message;
    }
}
