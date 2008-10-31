/*
 * $Id$
 */

package org.opendds.jms;

import java.io.Serializable;
import java.util.List;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import javax.jms.BytesMessage;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MapMessage;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.MessageProducer;
import javax.jms.ObjectMessage;
import javax.jms.Queue;
import javax.jms.QueueBrowser;
import javax.jms.Session;
import javax.jms.StreamMessage;
import javax.jms.TemporaryQueue;
import javax.jms.TemporaryTopic;
import javax.jms.TextMessage;
import javax.jms.Topic;
import javax.jms.TopicSubscriber;

import DDS.Subscriber;
import DDS.DomainParticipant;
import DDS.Publisher;
import DDS.NOT_NEW_VIEW_STATE;
import DDS.ANY_INSTANCE_STATE;
import DDS.ANY_SAMPLE_STATE;
import DDS.ANY_VIEW_STATE;
import DDS.SampleInfoSeqHolder;
import DDS.SampleInfo;
import DDS.RETCODE_OK;

import OpenDDS.JMS.MessagePayloadDataReader;
import OpenDDS.JMS.MessagePayloadSeqHolder;
import OpenDDS.JMS.MessagePayload;

import org.opendds.jms.util.Objects;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class SessionImpl implements Session {
    private int acknowledgeMode;
    private boolean transacted;
    private MessageListener messageListener;

    // JMS 1.1, 4.4.1, Created producers, need to close them when the session is closed
    private List<MessageProducer> createdProducers;
    // JMS 1.1, 4.4.1, Created consumers, need to close them when the session is closed
    private List<MessageConsumer> createdConsumers;
    // JMS 1.1, 4.4.14, Asynchronous Message delivery thread
    private ExecutorService messageDeliveryExecutorService;
    // JMS 1.1, 4.4.11, The sessions view of unacknowledged Messages, used in recover()
    private List<DataReaderHandlePair> unacknowledged;
    private Object lockForUnacknowledged;

    // OpenDDS stuff
    private DomainParticipant participant;
    private Publisher publisher;
    private Subscriber subscriber;

    public SessionImpl(boolean transacted, int acknowledgeMode, DomainParticipant participant, Publisher publisher, Subscriber subscriber) {
        Objects.ensureNotNull(participant);
        Objects.ensureNotNull(publisher);
        Objects.ensureNotNull(subscriber);

        this.transacted = transacted;
        this.acknowledgeMode = acknowledgeMode;
        this.subscriber = subscriber;
        this.publisher = publisher;
        this.participant = participant;

        this.createdProducers = new ArrayList<MessageProducer>();
        this.createdConsumers = new ArrayList<MessageConsumer>();
        this.messageDeliveryExecutorService = Executors.newSingleThreadExecutor();
        this.unacknowledged = new LinkedList<DataReaderHandlePair>();
        this.lockForUnacknowledged = new Object();
    }

    public int getAcknowledgeMode() {
        return acknowledgeMode;
    }

    public boolean getTransacted() throws JMSException {
        return transacted;
    }

    public MessageListener getMessageListener() throws JMSException {
        return messageListener;
    }

    public void setMessageListener(MessageListener messageListener) throws JMSException {
        // TODO what do a session level MessageListener do?
        this.messageListener = messageListener;
    }

    public MessageConsumer createConsumer(Destination destination) throws JMSException {
        return createConsumer(destination, null, false);
    }

    public MessageConsumer createConsumer(Destination destination,
                                          String messageSelector) throws JMSException {
        return createConsumer(destination, messageSelector, false);
    }

    public MessageConsumer createConsumer(Destination destination,
                                          String messageSelector,
                                          boolean noLocal) throws JMSException {
        MessageConsumer messageConsumer = new TopicMessageConsumerImpl(destination, messageSelector, noLocal, subscriber, participant, this);
        createdConsumers.add(messageConsumer);
        return messageConsumer;
    }

    public MessageProducer createProducer(Destination destination) throws JMSException {
        MessageProducer messageProducer = new TopicMessageProducerImpl(destination, publisher, participant);
        createdProducers.add(messageProducer);
        return messageProducer;
    }

    public TopicSubscriber createDurableSubscriber(Topic topic,
                                                   String name) throws JMSException {
        // TODO
        return null;
    }

    public TopicSubscriber createDurableSubscriber(Topic topic,
                                                   String name,
                                                   String messageSelector,
                                                   boolean noLocal) throws JMSException {
        // TODO
        return null;
    }

    public void unsubscribe(String name) throws JMSException {
        // TODO
    }

    public Queue createQueue(String queueName) throws JMSException {
        throw new IllegalStateException();
    }

    public TemporaryQueue createTemporaryQueue() throws JMSException {
        throw new IllegalStateException();
    }

    public QueueBrowser createBrowser(Queue queue) throws JMSException {
        throw new IllegalStateException();
    }

    public QueueBrowser createBrowser(Queue queue,
                                      String messageSelector) throws JMSException {

        throw new UnsupportedOperationException();
    }

    public Topic createTopic(String topicName) throws JMSException {
        // TODO
        return null;
    }

    public TemporaryTopic createTemporaryTopic() throws JMSException {
        // TODO
        return null;
    }

    public Message createMessage() throws JMSException {
        // TODO
        return null;
    }

    public BytesMessage createBytesMessage() throws JMSException {
        // TODO
        return null;
    }

    public MapMessage createMapMessage() throws JMSException {
        // TODO
        return null;
    }

    public ObjectMessage createObjectMessage() throws JMSException {
        return createObjectMessage(null);
    }

    public ObjectMessage createObjectMessage(Serializable object) throws JMSException {
        // TODO
        return null;
    }

    public StreamMessage createStreamMessage() throws JMSException {
        // TODO
        return null;
    }

    public TextMessage createTextMessage() throws JMSException {
        return createTextMessage(null);
    }

    public TextMessage createTextMessage(String text) throws JMSException {
        // TODO
        return null;
    }

    public void run() {
        throw new UnsupportedOperationException();
    }

    public void commit() throws JMSException {
        throw new IllegalStateException();
    }

    public void rollback() throws JMSException {
        throw new IllegalStateException();
    }

    public void recover() throws JMSException {
        // TODO
    }

    public void close() throws JMSException {
        // TODO
    }

    public ExecutorService getMessageDeliveryExecutorService() {
        return messageDeliveryExecutorService;
    }

    void doAcknowledge() {
        // TODO locking
        List<DataReaderHandlePair> copy;
        synchronized(lockForUnacknowledged) {
            copy = new LinkedList<DataReaderHandlePair>(unacknowledged);
            unacknowledged.clear();
        }
        for (DataReaderHandlePair pair: copy) {
            MessagePayloadDataReader dataReader = pair.getDataReader();
            int instanceHandle = pair.getInstanceHandle();

            MessagePayloadSeqHolder payloads = new MessagePayloadSeqHolder(new MessagePayload[0]);
            SampleInfoSeqHolder infos = new SampleInfoSeqHolder(new SampleInfo[0]);
            int rc = dataReader.take_instance(payloads, infos, 1, instanceHandle, ANY_SAMPLE_STATE.value, ANY_VIEW_STATE.value, ANY_INSTANCE_STATE.value);
            if (rc == RETCODE_OK.value) {
                // The instance handle is gone from the dataReader for some other reason.
            }
        }
    }

    public void addToUnacknowledged(DataReaderHandlePair dataReaderHandlePair) {
        synchronized(lockForUnacknowledged) {
            unacknowledged.add(dataReaderHandlePair);
        }
    }
}