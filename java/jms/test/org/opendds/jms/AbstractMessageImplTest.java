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

        final String testMessageID = "Test Message ID";
        message.setJMSMessageID(testMessageID);
        assertEquals(testMessageID, message.getJMSMessageID());

        long timestamp = System.currentTimeMillis();
        message.setJMSTimestamp(timestamp);
        assertEquals(timestamp, message.getJMSTimestamp());

        final String testCorrelationID = "Test Correlation ID";
        message.setJMSCorrelationID(testCorrelationID);
        assertEquals(testCorrelationID, message.getJMSCorrelationID());

        Destination destination2 = DestinationImpl.fromString("Test ReplyTo");
        message.setJMSReplyTo(destination2);
        assertEquals(destination2, message.getJMSReplyTo());

        message.setJMSRedelivered(true);
        assertEquals(true, message.getJMSRedelivered());
        message.setJMSRedelivered(false);
        assertEquals(false, message.getJMSRedelivered());

        final String testJMSType = "Test JMS Type";
        message.setJMSType(testJMSType);
        assertEquals(testJMSType, message.getJMSType());

        final long testExpiration = 9876543210L;
        message.setJMSExpiration(testExpiration);
        assertEquals(testExpiration, message.getJMSExpiration());

        final int testPriority = 5;
        message.setJMSPriority(testPriority);
        assertEquals(testPriority, message.getJMSPriority());
    }

    @Test
    public void testClearProperties() throws JMSException {
        Message message = new MapMessageImpl();

        message.setBooleanProperty("boolean", false);
        assertTrue(message.getPropertyNames().hasMoreElements());
        assertTrue(message.propertyExists("boolean"));

        message.clearProperties();
        assertFalse(message.getPropertyNames().hasMoreElements());
        assertFalse(message.propertyExists("boolean"));
    }

    @Test
    public void testSetAndGetProperties() throws JMSException {
        Message message = new StreamMessageImpl();

        message.setBooleanProperty("boolean", true);
        assertEquals(true, message.getBooleanProperty("boolean"));

        message.setByteProperty("byte", (byte) 1024);
        assertEquals((byte) 1024, message.getByteProperty("byte"));

        message.setShortProperty("short", (short) 2048);
        assertEquals((short) 2048, message.getShortProperty("short"));

        message.setIntProperty("int", 3072);
        assertEquals(3072, message.getIntProperty("int"));

        message.setLongProperty("long", 9765625L);
        assertEquals(9765625L, message.getLongProperty("long"));

        message.setFloatProperty("float", 3.14F);
        assertEquals(3.14F, message.getFloatProperty("float"), 1e-6);

        message.setDoubleProperty("double", 2.718281828459045);
        assertEquals(2.718281828459045, message.getDoubleProperty("double"), 1e-12);

        final String greeting = "Hello OpenDDS JMS Provider";
        message.setStringProperty("string", greeting);
        assertEquals(greeting, message.getStringProperty("string"));

        Object o = new Double(2.718281828459045);
        message.setObjectProperty("object", o);
        assertEquals(o, message.getObjectProperty("object"));
    }
}
