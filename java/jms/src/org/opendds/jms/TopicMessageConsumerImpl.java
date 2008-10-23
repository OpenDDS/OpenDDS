/*
 * $Id$
 */

package org.opendds.jms;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.Destination;
import DDS.Subscriber;
import DDS.DomainParticipant;
import org.opendds.jms.util.Objects;
import org.opendds.jms.util.Strings;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TopicMessageConsumerImpl implements MessageConsumer {
    private Destination destination;
    private String messageSelector;
    private MessageListener messageListener;

    // DDS related stuff
    private Subscriber subscriber;
    private DomainParticipant participant;
    
    private boolean closed;

    public TopicMessageConsumerImpl(Destination destination, Subscriber subscriber, DomainParticipant participant) {
        Objects.ensureNotNull(subscriber);
        Objects.ensureNotNull(participant);

        this.destination = destination;
        this.subscriber = subscriber;
        this.participant = participant;

        initConsumer();
    }

    private void initConsumer() {
        this.closed = false;
    }

    public String getMessageSelector() throws JMSException {
        return Strings.isEmpty(messageSelector) ? null : messageSelector;
    }

    public MessageListener getMessageListener() throws JMSException {
        return messageListener;
    }

    public void setMessageListener(MessageListener listener) throws JMSException {
        this.messageListener = listener;
    }

    public Message receive() throws JMSException {
        return null;
    }

    public Message receive(long timeout) throws JMSException {
        return null;
    }

    public Message receiveNoWait() throws JMSException {
        return null;
    }

    public void close() throws JMSException {
        // TODO
    }
}
