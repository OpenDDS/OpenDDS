package org.opendds.jms;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.MessageProducer;
import javax.jms.TextMessage;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
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
        if (dcpsInfoRepoRunning()) {
            final FakeObjects fakeObjects = createFakeObjects();
            // Order is significant
            doTestClose(fakeObjects);
            doTestConsumer(fakeObjects);
            doTestMessageListener(fakeObjects);
        }

    }

    private void doTestClose(FakeObjects fakeObjects) throws JMSException {
        final MessageConsumer messageConsumer = new TopicMessageConsumerImpl(fakeObjects.destination,
            fakeObjects.subscriber, fakeObjects.participant);
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

    private void doTestMessageListener(FakeObjects fakeObjects) throws JMSException {
        final MessageConsumer messageConsumer = new TopicMessageConsumerImpl(fakeObjects.destination,
            fakeObjects.subscriber, fakeObjects.participant);
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

    private void doTestConsumer(FakeObjects fakeObjects) throws JMSException {
        MessageConsumer messageConsumer = new TopicMessageConsumerImpl(fakeObjects.destination,
            fakeObjects.subscriber, fakeObjects.participant);
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
        assertNull(messageConsumer.receive());
        assertNull(messageConsumer.receiveNoWait());
        assertNull(messageConsumer.receive(100000L));

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

        typeSupport.register_type(participant, "OpenDDS::MessagePayload");
        final Topic topic = participant.create_topic("OpenDDS::MessagePayload", typeSupport.get_type_name(), TOPIC_QOS_DEFAULT.get(), null);
        assertNotNull(topic);

        Destination destination = new TopicImpl() {
            public Topic extractDDSTopic() {
                return topic;
            }
        };

        Publisher publisher = participant.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null);
        assertNotNull(publisher);

        TransportImpl transport2 = TheTransportFactory.create_transport_impl(2, TheTransportFactory.AUTO_CONFIG);
        assertNotNull(transport2);

        AttachStatus attachStatus2 = transport2.attach_to_publisher(publisher);
        assertNotNull(attachStatus2);

        TextMessage message = new TextMessageImpl();

        fakeObjects.destination = destination;
        fakeObjects.subscriber = subscriber;
        fakeObjects.participant = participant;

        fakeObjects.publisher = publisher;
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
                if (line.contains(":4096")) return true;
            }
            return false;
        } catch (IOException e) {
            return false;
        }
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
            messageProducer = new TopicMessageProducerImpl(fakeObjects.destination,
                fakeObjects.publisher, fakeObjects.participant);
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
}
