/*
 * $Id$
 */

package org.opendds.jms;

import java.lang.reflect.Field;
import java.util.Collection;
import java.util.List;
import java.util.Map;

import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.TextMessage;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
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

// TODO Remove this class once the functional tests in MessageConsumerImplTest are running
/**
 * @author  Weiqi Gao
 * @version $Revision$
 */
public class TopicMessageConsumerImplTest {
    @Test
    public void testDummy() {
        assertTrue(true);
    }

    @Test
    // Uncomment locally to run this test.  It depends on an externally running DCPSInfoRepo
    public void testConsumer() throws JMSException {
        // Hack, will use the new Java wrapper to start or stop the DCPSInfoRepo in a few days
        // wqg, Mon Oct 20 12:48:18 CDT 2008
        if (TestUtils.runWithInfoRepo()) {
            final FakeObjects fakeObjects = createFakeObjects();
            // Order is significant
            doTestClose(fakeObjects);
            doTestConsumer(fakeObjects);
            doTestMessageListener(fakeObjects);
            doTestAcknowledgement(fakeObjects);
        }

    }

    private void doTestClose(FakeObjects fakeObjects) throws JMSException {
        Session session = new SessionImpl(null, false, Session.AUTO_ACKNOWLEDGE);
        final MessageConsumer messageConsumer = session.createConsumer(fakeObjects.destination);
        assertNotNull(messageConsumer);
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
        assertNull(message);
        try {
            thread.join();
        } catch (InterruptedException e) {
            // Don't care
        }
    }

    private void doTestConsumer(FakeObjects fakeObjects) throws JMSException {
        Session session = new SessionImpl(null, false, Session.AUTO_ACKNOWLEDGE);
        final MessageConsumer messageConsumer = session.createConsumer(fakeObjects.destination);
        assertNotNull(messageConsumer);

        assertNull(messageConsumer.getMessageSelector());

        // Fake out a MessageProducer and pump some messages
        FakeMessageProducer fakeProducer = new FakeMessageProducer(fakeObjects);
        final Thread fakeProducerThread = new Thread(fakeProducer);
        fakeProducerThread.start();

        final Message message = messageConsumer.receive();
        assertNotNull(message);

        final Message message2 = messageConsumer.receive(100000L);
        assertNotNull(message2);

        final Message message3 = messageConsumer.receiveNoWait();
        assertNotNull(message3);

        final Message message4 = messageConsumer.receiveNoWait();
        assertNull(message4);

        messageConsumer.close();
        try {
            messageConsumer.receive();
            fail("Should throw");
        } catch (IllegalStateException e) {
            assertEquals("This MessageConsumer is closed.", e.getMessage());
        }

        try {
            messageConsumer.receiveNoWait();
        } catch (IllegalStateException e) {
            assertEquals("This MessageConsumer is closed.", e.getMessage());
        }

        try {
            messageConsumer.receive(100000L);
        } catch (IllegalStateException e) {
            assertEquals("This MessageConsumer is closed.", e.getMessage());
        }

        try {
            fakeProducerThread.join();
            fakeProducer.dispose();
        } catch (InterruptedException e) {
            // Don't care
        }
    }

    private void doTestMessageListener(FakeObjects fakeObjects) throws JMSException {
        Session session = new SessionImpl(null, false, Session.AUTO_ACKNOWLEDGE);
        final MessageConsumer messageConsumer = session.createConsumer(fakeObjects.destination);
        assertNotNull(messageConsumer);

        MyMessageListener messageListener = new MyMessageListener();
        messageConsumer.setMessageListener(messageListener);
        assertEquals(messageListener, messageConsumer.getMessageListener());

        messageConsumer.setMessageListener(null);
        assertEquals(null, messageConsumer.getMessageListener());

        messageConsumer.setMessageListener(messageListener);

        // Fake out a MessageProducer and pump some messages
        FakeMessageProducer fakeProducer = new FakeMessageProducer(fakeObjects);
        final Thread fakeProducerThread = new Thread(fakeProducer);
        fakeProducerThread.start();

        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            // Don't care
        }
        assertEquals(4, messageListener.getOnMessageCallCount());

        try {
            fakeProducerThread.join();
            fakeProducer.dispose();
        } catch (InterruptedException e) {
            // Don't care
        }
    }

    private void doTestAcknowledgement(FakeObjects fakeObjects) throws JMSException {
        ClientAcknowledgementSessionImpl session = new ClientAcknowledgementSessionImpl(fakeObjects);
        final MessageConsumer messageConsumer = session.createConsumer(fakeObjects.destination);

        // Fake out a MessageProducer and pump some messages
        FakeMessageProducer fakeProducer = new FakeMessageProducer(fakeObjects);
        final Thread fakeProducerThread = new Thread(fakeProducer);
        fakeProducerThread.start();

        final Message message = messageConsumer.receive();
        assertNotNull(message);

        final Message message2 = messageConsumer.receive(100000L);
        assertNotNull(message2);

        final Message message3 = messageConsumer.receiveNoWait();
        assertNull(message3);

        assertEquals(2, session.getUnacknowledgedCount());

        message.acknowledge();

        assertEquals(0, session.getUnacknowledgedCount());

        try {
            fakeProducerThread.join();
            fakeProducer.dispose();
        } catch (InterruptedException e) {
            // Don't care
        }
    }

    private FakeObjects createFakeObjects() {
        FakeObjects fakeObjects = new FakeObjects();

        String[] fakeArgs = new String[]{"-ORBSvcConf", "tcp.conf",
            "-ORBListenEndpoints", "iiop://127.0.0.1:12348",
            "-ORBDebugLevel", "10",
            "-DCPSDebugLevel", "10",
            "-ORBLogFile", "TopicMessageConsumerImplTest.log",
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

        final MessagePayloadTypeSupportImpl typeSupport = new MessagePayloadTypeSupportImpl();
        assertNotNull(typeSupport);

        typeSupport.register_type(participant, "OpenDDS::JMS::MessagePayload");
        final Topic topic = participant.create_topic("OpenDDS::JMS::MessagePayload", typeSupport.get_type_name(), TOPIC_QOS_DEFAULT.get(), null);
        assertNotNull(topic);

        Destination destination = new TopicImpl("Topic 1") {
            public Topic createTopic() {
                return topic;
            }
        };

        Publisher publisher = participant.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null);
        assertNotNull(publisher);

        TransportImpl transport2 = TheTransportFactory.create_transport_impl(2, TheTransportFactory.AUTO_CONFIG);
        assertNotNull(transport2);

        AttachStatus attachStatus2 = transport2.attach_to_publisher(publisher);
        assertNotNull(attachStatus2);

        TextMessage message = new TextMessageImpl(null);

        fakeObjects.destination = destination;
        fakeObjects.subscriber = subscriber;
        fakeObjects.participant = participant;

        fakeObjects.publisher = publisher;
        fakeObjects.message = message;

        return fakeObjects;
    }

    private static class FakeObjects {
        public Destination destination;
        public Subscriber subscriber;
        public DomainParticipant participant;

        // For FakeMessageProducer thread
        public Publisher publisher;
        public Message message;
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

    private static class FakeMessageProducer implements Runnable {
        private FakeObjects fakeObjects;
        private MessageProducer messageProducer;


        private FakeMessageProducer(FakeObjects fakeObjects) throws JMSException {
            this.fakeObjects = fakeObjects;
            Session session = new SessionImpl(null, false, Session.AUTO_ACKNOWLEDGE);
            messageProducer = session.createProducer(fakeObjects.destination);
        }

        public void run() {
            try {
                TextMessage textMessage = (TextMessage) fakeObjects.message;

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
            messageProducer.close();
        }
    }

    private static class ClientAcknowledgementSessionImpl extends SessionImpl {
        public ClientAcknowledgementSessionImpl(FakeObjects fakeObjects) {
            super(null, false, Session.CLIENT_ACKNOWLEDGE);
            // super(false, Session.CLIENT_ACKNOWLEDGE, fakeObjects.participant, fakeObjects.publisher, fakeObjects.subscriber, null);
        }

        @SuppressWarnings("unchecked")
        public int getUnacknowledgedCount() {
            try {
                final Field field = SessionImpl.class.getDeclaredField("unacknowledged");
                field.setAccessible(true);
                Map<MessageConsumerImpl, List<DataReaderHandlePair>> unacknowledged = (Map<MessageConsumerImpl, List<DataReaderHandlePair>>) field.get(this);
                final Collection<List<DataReaderHandlePair>> listCollection = unacknowledged.values();
                int count = 0;
                for (List<DataReaderHandlePair> pairs : listCollection) {
                    count += pairs.size();
                }
                return count;
            } catch (NoSuchFieldException e) {
                e.printStackTrace();
            } catch (IllegalAccessException e) {
                e.printStackTrace();
            }
            return Integer.MAX_VALUE; // Unexpected
        }
    }
}
