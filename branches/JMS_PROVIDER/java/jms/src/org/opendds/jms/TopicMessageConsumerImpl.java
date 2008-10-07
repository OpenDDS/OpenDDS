/*
 * $Id$
 */

package org.opendds.jms;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TopicMessageConsumerImpl implements MessageConsumer {

    public String getMessageSelector() throws JMSException {
        return null;
    }

    public MessageListener getMessageListener() throws JMSException {
        return null;
    }

    public void setMessageListener(MessageListener listener) throws JMSException {
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
    }
}
