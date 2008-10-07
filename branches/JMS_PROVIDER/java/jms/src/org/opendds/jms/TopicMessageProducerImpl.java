/*
 * $Id$
 */

package org.opendds.jms;

import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageProducer;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TopicMessageProducerImpl implements MessageProducer {

    public Destination getDestination() throws JMSException {
        return null;
    }

    public boolean getDisableMessageID() throws JMSException {
        return false;
    }

    public void setDisableMessageID(boolean value) throws JMSException {
    }

    public boolean getDisableMessageTimestamp() throws JMSException {
        return false;
    }

    public void setDisableMessageTimestamp(boolean value) throws JMSException {
    }

    public int getDeliveryMode() throws JMSException {
        return 0;
    }

    public void setDeliveryMode(int deliveryMode) throws JMSException {
    }

    public int getPriority() throws JMSException {
        return 0;
    }

    public void setPriority(int defaultPriority) throws JMSException {
    }

    public long getTimeToLive() throws JMSException {
        return 0;
    }

    public void setTimeToLive(long timeToLive) throws JMSException {
    }

    public void send(Message message) throws JMSException {
    }

    public void send(Message message,
                     int deliveryMode,
                     int priority,
                     long timeToLive) throws JMSException {
    }

    public void send(Destination destination, Message message) throws JMSException {
    }

    public void send(Destination destination,
                     Message message,
                     int deliveryMode,
                     int priority,
                     long timeToLive) throws JMSException {
    }

    public void close() throws JMSException {
    }
}
