package org.opendds.jms;

import static org.junit.Assert.*;
import org.junit.Test;

import javax.jms.StreamMessage;
import javax.jms.MapMessage;
import javax.jms.TextMessage;
import javax.jms.ObjectMessage;
import javax.jms.BytesMessage;
import javax.jms.Message;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.DeliveryMode;

public class AbstractMessageImplTest {

    @Test
    public void testCreatingMessages() {
        StreamMessage streamMessage = new StreamMessageImpl();
        MapMessage mapMessage = new MapMessageImpl();
        TextMessage textMessage = new TextMessageImpl();
        ObjectMessage objectMessage = new ObjectMessageImpl();
        BytesMessage bytesMessage = new BytesMessageImpl();

        assertNotNull(streamMessage);
        assertNotNull(mapMessage);
        assertNotNull(textMessage);
        assertNotNull(objectMessage);
        assertNotNull(bytesMessage);
    }

    @Test
    public void testSettingAndGettingHeaders() throws JMSException {
        Message message = new TextMessageImpl();

        Destination destination = DestinationImpl.fromString("Test Destination");
        message.setJMSDestination(destination);
        assertEquals(destination, message.getJMSDestination());

        message.setJMSDeliveryMode(DeliveryMode.NON_PERSISTENT);
        assertEquals(DeliveryMode.NON_PERSISTENT, message.getJMSDeliveryMode());
        message.setJMSDeliveryMode(DeliveryMode.PERSISTENT);
        assertEquals(DeliveryMode.PERSISTENT, message.getJMSDeliveryMode());

        message.setJMSMessageID("Test Message ID");
        assertEquals("Test Message ID", message.getJMSMessageID());

        long timestamp = System.currentTimeMillis();
        message.setJMSTimestamp(timestamp);
        assertEquals(timestamp, message.getJMSTimestamp());

        message.setJMSCorrelationID("Test Correlation ID");
        assertEquals("Test Correlation ID", message.getJMSCorrelationID());

        Destination destination2 = DestinationImpl.fromString("Test ReplyTo");
        message.setJMSReplyTo(destination2);
        assertEquals(destination2, message.getJMSReplyTo());

        message.setJMSRedelivered(true);
        assertEquals(true, message.getJMSRedelivered());
        message.setJMSRedelivered(false);
        assertEquals(false, message.getJMSRedelivered());

        message.setJMSType("Test JMS Type");
        assertEquals("Test JMS Type", message.getJMSType());

        message.setJMSExpiration(9876543210L);
        assertEquals(9876543210L, message.getJMSExpiration());

        message.setJMSPriority(5);
        assertEquals(5, message.getJMSPriority());
    }
}
