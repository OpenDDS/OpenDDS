/*
 * $Id$
 */

package org.opendds.jms;

import java.io.Serializable;
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

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class SessionImpl implements Session {

    public int getAcknowledgeMode() throws JMSException {
        return 0;
    }

    public boolean getTransacted() throws JMSException {
        return false;
    }

    public MessageListener getMessageListener() throws JMSException {
        return null;
    }

    public void setMessageListener(MessageListener listener) throws JMSException {
    }

    public MessageConsumer createConsumer(Destination destination) throws JMSException {
        return null;
    }

    public MessageConsumer createConsumer(Destination destination,
                                          String messageSelector) throws JMSException {
        return null;
    }

    public MessageConsumer createConsumer(Destination destination,
                                          String messageSelector,
                                          boolean noLocal) throws JMSException {
        return null;
    }

    public MessageProducer createProducer(Destination destination) throws JMSException {
        return null;
    }

    public TopicSubscriber createDurableSubscriber(Topic topic,
                                                   String name) throws JMSException {
        return null;
    }

    public TopicSubscriber createDurableSubscriber(Topic topic,
                                                   String name,
                                                   String messageSelector,
                                                   boolean noLocal) throws JMSException {
        return null;
    }

    public void unsubscribe(String name) throws JMSException {
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
        return null;
    }

    public TemporaryTopic createTemporaryTopic() throws JMSException {
        return null;
    }

    public Message createMessage() throws JMSException {
        return null;
    }

    public BytesMessage createBytesMessage() throws JMSException {
        return null;
    }

    public MapMessage createMapMessage() throws JMSException {
        return null;
    }

    public ObjectMessage createObjectMessage() throws JMSException {
        return createObjectMessage(null);
    }

    public ObjectMessage createObjectMessage(Serializable object) throws JMSException {
        return null;
    }

    public StreamMessage createStreamMessage() throws JMSException {
        return null;
    }

    public TextMessage createTextMessage() throws JMSException {
        return createTextMessage(null);
    }

    public TextMessage createTextMessage(String text) throws JMSException {
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
    }

    public void close() throws JMSException {
    }
}
