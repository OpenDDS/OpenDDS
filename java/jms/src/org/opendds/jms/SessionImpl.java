/*
 * $Id$
 */

package org.opendds.jms;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Executor;

import javax.jms.BytesMessage;
import javax.jms.Destination;
import javax.jms.InvalidDestinationException;
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

import DDS.ANY_INSTANCE_STATE;
import DDS.ANY_SAMPLE_STATE;
import DDS.ANY_VIEW_STATE;
import DDS.DomainParticipant;
import DDS.RETCODE_OK;
import DDS.SampleInfo;
import DDS.SampleInfoSeqHolder;
import OpenDDS.JMS.MessagePayload;
import OpenDDS.JMS.MessagePayloadDataReader;
import OpenDDS.JMS.MessagePayloadSeqHolder;

/**
 * @author  Weiqi Gao
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
    private MessageDeliveryExecutor executor;
    // JMS 1.1, 4.4.11, The sessions view of unacknowledged Messages, used in recover()
    private Map<MessageConsumerImpl, List<DataReaderHandlePair>> unacknowledged;
    private final Object lockForUnacknowledged;
    private Map<MessageConsumerImpl, List<DataReaderHandlePair>> toBeRecovered;
    private ConnectionImpl owningConnection;

    // OpenDDS stuff
    private DomainParticipant participant;

    private boolean closed;
    private Object lockForClosed;

    public SessionImpl(ConnectionImpl owningConnection, boolean transacted, int acknowledgeMode) {
        this.owningConnection = owningConnection;
        this.transacted = transacted;
        this.acknowledgeMode = acknowledgeMode;

        this.participant = owningConnection.getParticipant();

        this.closed = false;

        this.createdProducers = new ArrayList<MessageProducer>();
        this.createdConsumers = new ArrayList<MessageConsumer>();
        this.executor = new MessageDeliveryExecutor(owningConnection);
        this.unacknowledged = new HashMap<MessageConsumerImpl, List<DataReaderHandlePair>>();
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
        checkClosed();
        // TODO what do a session level MessageListener do?
        this.messageListener = messageListener;
    }

    public MessageConsumer createConsumer(Destination destination) throws JMSException {
        checkClosed();
        return createConsumer(destination, null, false);
    }

    public MessageConsumer createConsumer(Destination destination,
                                          String messageSelector) throws JMSException {
        checkClosed();
        return createConsumer(destination, messageSelector, false);
    }

    public MessageConsumer createConsumer(Destination destination,
                                          String messageSelector,
                                          boolean noLocal) throws JMSException {
        checkClosed();
        validateDestination(destination);
        MessageConsumer messageConsumer = new MessageConsumerImpl(this, destination, messageSelector, noLocal);
        createdConsumers.add(messageConsumer);
        return messageConsumer;
    }

    private void validateDestination(Destination destination) throws InvalidDestinationException {
        if (!(destination instanceof TopicImpl)) {
            throw new InvalidDestinationException("An invalid destination is supplied: " + destination + ".");
        }
    }

    public MessageProducer createProducer(Destination destination) throws JMSException {
        checkClosed();
        MessageProducer messageProducer = new MessageProducerImpl(this, destination);
        createdProducers.add(messageProducer);
        return messageProducer;
    }

    public TopicSubscriber createDurableSubscriber(Topic topic,
                                                   String name) throws JMSException {
        checkClosed();
        return createDurableSubscriber(topic, name, null, false);
    }

    public TopicSubscriber createDurableSubscriber(Topic topic,
                                                   String name,
                                                   String messageSelector,
                                                   boolean noLocal) throws JMSException {
        checkClosed();
        // TODO
        return null;
    }

    public void unsubscribe(String name) throws JMSException {
        checkClosed();
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
        checkClosed();
        return new TopicImpl(topicName);
    }

    public TemporaryTopic createTemporaryTopic() throws JMSException {
        checkClosed();
        return owningConnection.createTemporaryTopic();
    }

    public Message createMessage() throws JMSException {
        checkClosed();
        return new TextMessageImpl(this);
    }

    public BytesMessage createBytesMessage() throws JMSException {
        checkClosed();
        return new BytesMessageImpl(this);
    }

    public MapMessage createMapMessage() throws JMSException {
        checkClosed();
        return new MapMessageImpl(this);
    }

    public ObjectMessage createObjectMessage() throws JMSException {
        checkClosed();
        return new ObjectMessageImpl(this);
    }

    public ObjectMessage createObjectMessage(Serializable object) throws JMSException {
        checkClosed();
        ObjectMessageImpl message = new ObjectMessageImpl(this);
        message.setObject(object);
        return message;
    }

    public StreamMessage createStreamMessage() throws JMSException {
        checkClosed();
        return new StreamMessageImpl(this);
    }

    public TextMessage createTextMessage() throws JMSException {
        checkClosed();
        return new TextMessageImpl(this);
    }

    public TextMessage createTextMessage(String text) throws JMSException {
        checkClosed();
        TextMessageImpl message = new TextMessageImpl(this);
        message.setText(text);
        return message;
    }

    public void run() {
        checkClosed();
        // TODO
    }

    public void commit() throws JMSException {
        throw new IllegalStateException();
    }

    public void rollback() throws JMSException {
        throw new IllegalStateException();
    }

    public void recover() throws JMSException {
        checkClosed();
        synchronized(lockForUnacknowledged) {
            toBeRecovered = new HashMap<MessageConsumerImpl, List<DataReaderHandlePair>>(unacknowledged);
            unacknowledged.clear();
        }
        if (transacted) throw new IllegalStateException("recover() called on a transacted Session.");
        stopMessageDelivery();
        if (messageListener != null) {
            recoverAsync();
        } else {
            for (MessageConsumerImpl consumer: toBeRecovered.keySet()) {
                final List<DataReaderHandlePair> pairs = toBeRecovered.get(consumer);
                consumer.doRecover(pairs);
            }
        }
    }

    private void recoverAsync() {
        // TODO
    }

    private void stopMessageDelivery() {
        // TODO should tell consumers to stop message delivery
    }

    public void close() throws JMSException {
        if (closed) return;
        for (MessageProducer producer: createdProducers) producer.close();
        for (MessageConsumer consumer: createdConsumers) consumer.close();
        executor.shutdown();
        this.closed = true;
    }

    public void checkClosed() {
        if (closed) throw new IllegalStateException("This Session is closed.");
    }

    Executor getMessageDeliveryExecutor() {
        return executor;
    }

    void doAcknowledge() {
        Map<MessageConsumerImpl, List<DataReaderHandlePair>> copy;
        synchronized(lockForUnacknowledged) {
            copy = new HashMap<MessageConsumerImpl, List<DataReaderHandlePair>>(unacknowledged);
            unacknowledged.clear();
        }
        for (List<DataReaderHandlePair> pairList: copy.values()) {
            for (DataReaderHandlePair pair: pairList) {
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
    }

    void addToUnacknowledged(DataReaderHandlePair dataReaderHandlePair, MessageConsumerImpl consumer) {
        synchronized(lockForUnacknowledged) {
            List<DataReaderHandlePair> pairs = unacknowledged.get(consumer);
            if (pairs == null) {
                pairs = new ArrayList<DataReaderHandlePair>();
                unacknowledged.put(consumer, pairs);
            }
            pairs.add(dataReaderHandlePair);
        }
    }

    ConnectionImpl getOwningConnection() {
        return owningConnection;
    }

    DomainParticipant getParticipant() {
        return participant;
    }
}