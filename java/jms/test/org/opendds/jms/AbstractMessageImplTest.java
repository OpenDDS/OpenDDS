/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.Enumeration;
import java.util.HashSet;
import java.util.Set;

import javax.jms.BytesMessage;
import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MapMessage;
import javax.jms.Message;
import javax.jms.MessageFormatException;
import javax.jms.MessageNotWriteableException;
import javax.jms.ObjectMessage;
import javax.jms.StreamMessage;
import javax.jms.TextMessage;
import javax.resource.ResourceException;
import javax.security.auth.Subject;

import org.junit.Test;
import org.omg.CORBA.StringSeqHolder;
import org.opendds.jms.resource.ConnectionRequestInfoImpl;
import org.opendds.jms.resource.ManagedConnectionImpl;

import DDS.DomainParticipant;
import DDS.DomainParticipantFactory;
import DDS.PARTICIPANT_QOS_DEFAULT;
import DDS.PUBLISHER_QOS_DEFAULT;
import DDS.Publisher;
import DDS.SUBSCRIBER_QOS_DEFAULT;
import DDS.Subscriber;
import DDS.TOPIC_QOS_DEFAULT;
import DDS.Topic;
import OpenDDS.DCPS.DEFAULT_STATUS_MASK;
import OpenDDS.DCPS.TheParticipantFactory;
import OpenDDS.JMS.MessagePayloadTypeSupportImpl;

/**
 * @author  Weiqi Gao
 */
public class AbstractMessageImplTest {
    private static final float FLOAT_EPSILON = 1e-6f;
    private static final double DOUBLE_EPSILON = 1e-12;

    @Test
    public void testCreatingMessages() {
        StreamMessage streamMessage = new StreamMessageImpl(null);
        MapMessage mapMessage = new MapMessageImpl(null);
        TextMessage textMessage = new TextMessageImpl(null);
        ObjectMessage objectMessage = new ObjectMessageImpl(null);
        BytesMessage bytesMessage = new BytesMessageImpl(null);

        assertNotNull(streamMessage);
        assertNotNull(mapMessage);
        assertNotNull(textMessage);
        assertNotNull(objectMessage);
        assertNotNull(bytesMessage);
    }

    @Test
    public void testSettingAndGettingNonDestinationHeaders() throws JMSException {
        Message message = new TextMessageImpl(null);

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
        AbstractMessageImpl message = new MapMessageImpl(null);

        message.setBooleanProperty("boolean", false);
        assertTrue(message.getPropertyNames().hasMoreElements());
        assertTrue(message.propertyExists("boolean"));

        message.clearProperties();
        assertFalse(message.getPropertyNames().hasMoreElements());
        assertFalse(message.propertyExists("boolean"));

        // JMS 1.1, 3.10
        message.setBooleanProperty("boolean", false);
        message.setPropertiesState(new MessageStatePropertiesNonWritable(message));
        assertTrue(message.getPropertyNames().hasMoreElements());
        assertTrue(message.propertyExists("boolean"));

        message.clearProperties();
        assertFalse(message.getPropertyNames().hasMoreElements());
        assertFalse(message.propertyExists("boolean"));
    }

    @Test
    public void testSetAndGetProperties() throws JMSException {
        Message message = new StreamMessageImpl(null);

        message.setBooleanProperty("boolean", true);
        assertEquals(true, message.getBooleanProperty("boolean"));

        message.setByteProperty("byte", (byte) 63);
        assertEquals((byte) 63, message.getByteProperty("byte"));

        message.setShortProperty("short", (short) 1024);
        assertEquals((short) 1024, message.getShortProperty("short"));

        message.setIntProperty("int", 2048);
        assertEquals(2048, message.getIntProperty("int"));

        message.setLongProperty("long", 9765625L);
        assertEquals(9765625L, message.getLongProperty("long"));

        message.setFloatProperty("float", 3.14F);
        assertEquals(3.14F, message.getFloatProperty("float"), FLOAT_EPSILON);

        message.setDoubleProperty("double", 2.718281828459045);
        assertEquals(2.718281828459045, message.getDoubleProperty("double"), DOUBLE_EPSILON);

        final String greeting = "Hello OpenDDS JMS Provider";
        message.setStringProperty("string", greeting);
        assertEquals(greeting, message.getStringProperty("string"));

        Object o = new Double(2.718281828459045);
        message.setObjectProperty("object", o);
        assertEquals(o, message.getObjectProperty("object"));
    }

    @Test
    public void testUpdatingProperties() throws JMSException {
        Message message = new StreamMessageImpl(null);
        populateProperties(message);

        message.setBooleanProperty("boolean", false);
        assertEquals(false, message.getBooleanProperty("boolean"));

        message.setByteProperty("byte", (byte) 127);
        assertEquals((byte) 127, message.getByteProperty("byte"));

        message.setShortProperty("short", (short) 10240);
        assertEquals((short) 10240, message.getShortProperty("short"));

        message.setIntProperty("int", 20480);
        assertEquals(20480, message.getIntProperty("int"));

        message.setLongProperty("long", 9765626L);
        assertEquals(9765626L, message.getLongProperty("long"));

        message.setFloatProperty("float", 6.28F);
        assertEquals(6.28F, message.getFloatProperty("float"), FLOAT_EPSILON);

        message.setDoubleProperty("double", 27.18281828459045);
        assertEquals(27.18281828459045, message.getDoubleProperty("double"), DOUBLE_EPSILON);

        final String greeting = "Goodbye OpenDDS JMS Provider";
        message.setStringProperty("string", greeting);
        assertEquals(greeting, message.getStringProperty("string"));

        Object o = new Long(System.currentTimeMillis());
        message.setObjectProperty("object", o);
        assertEquals(o, message.getObjectProperty("object"));
    }

    private void populateProperties(Message message) throws JMSException {
        message.setBooleanProperty("boolean", true);
        message.setByteProperty("byte", (byte) 63);
        message.setShortProperty("short", (short) 1024);
        message.setIntProperty("int", 2048);
        message.setLongProperty("long", 9765625L);
        message.setFloatProperty("float", 3.14F);
        message.setDoubleProperty("double", 2.718281828459045);
        final String greeting = "Hello OpenDDS JMS Provider";
        message.setStringProperty("string", greeting);
        Object o = new Double(2.718281828459045);
        message.setObjectProperty("object", o);
    }

    /**
     * JMS 1.1, 3.5.3
     */
    @Test
    public void testSetPropertiesInNotWritableState() {
        AbstractMessageImpl message = new StreamMessageImpl(null);
        message.setPropertiesState(new MessageStatePropertiesNonWritable(message));

        try {
            message.setBooleanProperty("boolean", true);
            fail("setBooleanProperty() succeded on a message in a non-writable propertiesState");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            message.setByteProperty("byte", (byte) 63);
            fail("setByteProperty() succeded on a message in a non-writable propertiesState");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            message.setShortProperty("short", (short) 1024);
            fail("setShortProperty() succeded on a message in a non-writable propertiesState");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            message.setIntProperty("int", 2048);
            fail("setIntProperty() succeded on a message in a non-writable propertiesState");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            message.setLongProperty("long", 9765625L);
            fail("setLongProperty() succeded on a message in a non-writable propertiesState");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            message.setFloatProperty("float", 3.14F);
            fail("setFloatProperty() succeded on a message in a non-writable propertiesState");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            message.setDoubleProperty("double", 2.718281828459045);
            fail("setDoubleProperty() succeded on a message in a non-writable propertiesState");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            final String greeting = "Hello OpenDDS JMS Provider";
            message.setStringProperty("string", greeting);
            fail("setStringProperty() succeded on a message in a non-writable propertiesState");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            Object o = new Double(2.718281828459045);
            message.setObjectProperty("object", o);
            fail("setObjectProperty() succeded on a message in a non-writable propertiesState");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
    }

    /**
     * JMS 1.1, 3.5.4
     */
    @Test
    public void testPropertyValueConversion() throws JMSException {
        Message message = new BytesMessageImpl(null);
        populateProperties(message);

        // Successfull conversions
        assertEquals("true", message.getStringProperty("boolean"));

        assertEquals((short) 63, message.getShortProperty("byte"));
        assertEquals(63, message.getIntProperty("byte"));
        assertEquals(63L, message.getLongProperty("byte"));
        assertEquals("63", message.getStringProperty("byte"));

        assertEquals(1024, message.getIntProperty("short"));
        assertEquals(1024L, message.getLongProperty("short"));
        assertEquals("1024", message.getStringProperty("short"));

        assertEquals(2048L, message.getLongProperty("int"));
        assertEquals("2048", message.getStringProperty("int"));

        assertEquals("9765625", message.getStringProperty("long"));

        assertEquals(3.14, message.getDoubleProperty("float"), FLOAT_EPSILON);
        assertEquals("3.14", message.getStringProperty("float"));

        assertEquals("2.718281828459045", message.getStringProperty("double"));

        message.setStringProperty("string", "true");
        assertEquals(true, message.getBooleanProperty("string"));

        message.setStringProperty("string", "63");
        assertEquals((byte) 63, message.getByteProperty("string"));

        message.setStringProperty("string", "1024");
        assertEquals((short) 1024, message.getShortProperty("string"));

        message.setStringProperty("string", "2048");
        assertEquals(2048, message.getIntProperty("string"));

        message.setStringProperty("string", "9765625");
        assertEquals(9765625L, message.getLongProperty("string"));

        message.setStringProperty("string", "3.14");
        assertEquals(3.14, message.getFloatProperty("string"), FLOAT_EPSILON);

        message.setStringProperty("string", "2.718281828459045");
        assertEquals(2.718281828459045, message.getDoubleProperty("string"), DOUBLE_EPSILON);

        // Illegal conversions
        cannotConvertToByte(message, "boolean");
        cannotConvertToShort(message, "boolean");
        cannotConvertToInt(message, "boolean");
        cannotConvertToLong(message, "boolean");
        cannotConvertToFloat(message, "boolean");
        cannotConvertToDouble(message, "boolean");

        cannotConvertToBoolean(message, "byte");
        cannotConvertToFloat(message, "byte");
        cannotConvertToDouble(message, "byte");

        cannotConvertToBoolean(message, "short");
        cannotConvertToByte(message, "short");
        cannotConvertToFloat(message, "short");
        cannotConvertToDouble(message, "short");

        cannotConvertToBoolean(message, "int");
        cannotConvertToByte(message, "int");
        cannotConvertToShort(message, "int");
        cannotConvertToFloat(message, "int");
        cannotConvertToDouble(message, "int");

        cannotConvertToBoolean(message, "long");
        cannotConvertToByte(message, "long");
        cannotConvertToShort(message, "long");
        cannotConvertToInt(message, "long");
        cannotConvertToFloat(message, "long");
        cannotConvertToDouble(message, "long");

        cannotConvertToBoolean(message, "float");
        cannotConvertToByte(message, "float");
        cannotConvertToShort(message, "float");
        cannotConvertToInt(message, "float");
        cannotConvertToLong(message, "float");

        cannotConvertToBoolean(message, "double");
        cannotConvertToByte(message, "double");
        cannotConvertToShort(message, "double");
        cannotConvertToInt(message, "double");
        cannotConvertToLong(message, "double");
        cannotConvertToFloat(message, "double");
    }

    private void cannotConvertToBoolean(Message message, String s) throws JMSException {
        try {
            message.getBooleanProperty(s);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Cannot convert property to boolean", e.getMessage());
        }
    }

    private void cannotConvertToByte(Message message, String s) throws JMSException {
        try {
            message.getByteProperty(s);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Cannot convert property to byte", e.getMessage());
        }
    }

    private void cannotConvertToShort(Message message, String s) throws JMSException {
        try {
            message.getShortProperty(s);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Cannot convert property to short", e.getMessage());
        }
    }

    private void cannotConvertToInt(Message message, String s) throws JMSException {
        try {
            message.getIntProperty(s);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Cannot convert property to int", e.getMessage());
        }
    }

    private void cannotConvertToLong(Message message, String s) throws JMSException {
        try {
            message.getLongProperty(s);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Cannot convert property to long", e.getMessage());
        }
    }

    private void cannotConvertToFloat(Message message, String s) throws JMSException {
        try {
            message.getFloatProperty(s);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Cannot convert property to float", e.getMessage());
        }
    }

    private void cannotConvertToDouble(Message message, String s) throws JMSException {
        try {
            message.getDoubleProperty(s);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Cannot convert property to double", e.getMessage());
        }
    }

    /**
     * JMS 1.1, 3.5.4
     */
    @Test
    public void testPropertyValueConversionFromNull() throws JMSException {
        Message message = new TextMessageImpl(null);
        message.setStringProperty("string", null);

        assertEquals(false, message.getBooleanProperty("string"));

        try {
            message.getByteProperty("string");
            fail("Should throw");
        } catch (NumberFormatException e) {
            assertEquals("null", e.getMessage());
        }

        try {
            message.getShortProperty("string");
            fail("Should throw");
        } catch (NumberFormatException e) {
            assertEquals("null", e.getMessage());
        }

        try {
            message.getIntProperty("string");
            fail("Should throw");
        } catch (NumberFormatException e) {
            assertEquals("null", e.getMessage());
        }

        try {
            message.getLongProperty("string");
            fail("Should throw");
        } catch (NumberFormatException e) {
            assertEquals("null", e.getMessage());
        }

        try {
            message.getFloatProperty("string");
            fail("Should throw");
        } catch (NullPointerException e) {
            assertNull(e.getMessage());
        }

        try {
            message.getDoubleProperty("string");
            fail("Should throw");
        } catch (NullPointerException e) {
            assertNull(e.getMessage());
        }
    }

    /**
     * JMS 1.1, 3.5.5
     */
    @Test
    public void testSetObjectProperty() throws JMSException {
        Message message = new ObjectMessageImpl(null);

        message.setObjectProperty("object", Boolean.TRUE);
        assertEquals(true, message.getBooleanProperty("object"));

        message.setObjectProperty("object", Byte.valueOf("63"));
        assertEquals((byte)63, message.getByteProperty("object"));

        message.setObjectProperty("object", Short.valueOf((short) 1024));
        assertEquals((short) 1024, message.getShortProperty("object"));

        message.setObjectProperty("object", Integer.valueOf(2048));
        assertEquals(2048, message.getIntProperty("object"));

        message.setObjectProperty("object", Long.valueOf(9765625));
        assertEquals(9765625L, message.getLongProperty("object"));

        message.setObjectProperty("object", Float.valueOf(3.14f));
        assertEquals(3.14f, message.getFloatProperty("object"), FLOAT_EPSILON);

        message.setObjectProperty("object", Double.valueOf(2.718281828459045));
        assertEquals(2.718281828459045, message.getDoubleProperty("object"), DOUBLE_EPSILON);

        final String greeting = "Hello OpenDDS JMS Provider";
        message.setObjectProperty("object", greeting);
        assertEquals(greeting, message.getStringProperty("object"));

        try {
            message.setObjectProperty("object", null);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Invalid value passed in for setObjectProperty()", e.getMessage());
        }

    }

    /**
     * JMS 1.1, 3.5.5
     */
    @Test
    public void testGetObjectProperty() throws JMSException {
        Message message = new BytesMessageImpl(null);
        populateProperties(message);

        assertEquals(Boolean.TRUE, message.getObjectProperty("boolean"));
        assertEquals(Byte.valueOf((byte)63), message.getObjectProperty("byte"));
        assertEquals(Short.valueOf((short) 1024), message.getObjectProperty("short"));
        assertEquals(Integer.valueOf(2048), message.getObjectProperty("int"));
        assertEquals(Long.valueOf(9765625), message.getObjectProperty("long"));
        assertEquals(Float.valueOf(3.14f), message.getObjectProperty("float"));
        assertEquals(Double.valueOf(2.718281828459045), message.getObjectProperty("double"));
        assertEquals("Hello OpenDDS JMS Provider", message.getObjectProperty("string"));

        assertNull(message.getObjectProperty("nonexistent"));
    }

    @Test
    public void testGetPropertyNames() throws JMSException {
        Message message = new TextMessageImpl(null);
        populateProperties(message);

        final Enumeration propertyNames = message.getPropertyNames();
        Set<String> names = new HashSet<String>();
        while (propertyNames.hasMoreElements()) {
            String name = (String) propertyNames.nextElement();
            names.add(name);
        }

        assertEquals(9, names.size());
        assertTrue(names.contains("boolean"));
        assertTrue(names.contains("byte"));
        assertTrue(names.contains("short"));
        assertTrue(names.contains("int"));
        assertTrue(names.contains("long"));
        assertTrue(names.contains("float"));
        assertTrue(names.contains("double"));
        assertTrue(names.contains("string"));
        assertTrue(names.contains("object"));
   }

    @Test
    public void testGettingNonExistentProperties() throws JMSException {
        Message message = new StreamMessageImpl(null);

        assertEquals(false, message.getBooleanProperty("nonexistent"));

        try {
            message.getByteProperty("nonexistent");
            fail("Should throw");
        } catch (NumberFormatException e) {
            assertEquals("null", e.getMessage());
        }

        try {
            message.getShortProperty("nonexistent");
            fail("Should throw");
        } catch (NumberFormatException e) {
            assertEquals("null", e.getMessage());
        }

        try {
            message.getIntProperty("nonexistent");
            fail("Should throw");
        } catch (NumberFormatException e) {
            assertEquals("null", e.getMessage());
        }

        try {
            message.getLongProperty("nonexistent");
            fail("Should throw");
        } catch (NumberFormatException e) {
            assertEquals("null", e.getMessage());
        }

        try {
            message.getFloatProperty("nonexistent");
            fail("Should throw");
        } catch (NullPointerException e) {
            assertNull(e.getMessage());
        }

        try {
            message.getDoubleProperty("nonexistent");
            fail("Should throw");
        } catch (NullPointerException e) {
            assertNull(e.getMessage());
        }

        assertNull(message.getStringProperty("nonexistent"));
        assertNull(message.getObjectProperty("nonexistent"));
    }

    @Test
    public void testSettingPropertiesWithIllegalNames() throws JMSException {
        Message message = new BytesMessageImpl(null);

        try {
            message.setBooleanProperty("illegal name", true);
            fail("Should throw");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalArgumentException);
        }

        try {
            message.setByteProperty("illegal name", (byte) 63);
            fail("Should throw");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalArgumentException);
        }

        try {
            message.setShortProperty("illegal name", (short) 1024);
            fail("Should throw");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalArgumentException);
        }

        try {
            message.setIntProperty("illegal name", 2048);
            fail("Should throw");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalArgumentException);
        }

        try {
            message.setLongProperty("illegal name", 9765625);
            fail("Should throw");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalArgumentException);
        }

        try {
            message.setFloatProperty("illegal name", 3.14f);
            fail("Should throw");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalArgumentException);
        }

        try {
            message.setDoubleProperty("illegal name", 2.718281828459045);
            fail("Should throw");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalArgumentException);
        }

        try {
            message.setStringProperty("illegal name", "Hello OpenDDS JMS Provider");
            fail("Should throw");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalArgumentException);
        }

        try {
            message.setObjectProperty("illegal name", new Integer(4096));
            fail("Should throw");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalArgumentException);
        }
    }

    @Test
    public void testGettingPropertiesWithIllegalNames() {
        Message message = new TextMessageImpl(null);

        try {
            message.getBooleanProperty("illegal name");
            fail("Should throw");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalArgumentException);
        }

        try {
            message.getByteProperty("illegal name");
            fail("Should throw");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalArgumentException);
        }

        try {
            message.getShortProperty("illegal name");
            fail("Should throw");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalArgumentException);
        }

        try {
            message.getIntProperty("illegal name");
            fail("Should throw");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalArgumentException);
        }

        try {
            message.getLongProperty("illegal name");
            fail("Should throw");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalArgumentException);
        }

        try {
            message.getFloatProperty("illegal name");
            fail("Should throw");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalArgumentException);
        }

        try {
            message.getDoubleProperty("illegal name");
            fail("Should throw");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalArgumentException);
        }

        try {
            message.getStringProperty("illegal name");
            fail("Should throw");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalArgumentException);
        }

        try {
            message.getObjectProperty("illegal name");
            fail("Should throw");
        } catch (Exception e) {
            assertTrue(e instanceof IllegalArgumentException);
        }
    }

    private FakeObjects createFakeObjects() throws ResourceException {
        FakeObjects fakeObjects = new FakeObjects();

        String[] fakeArgs = new String[]{"-ORBSvcConf", "tcp.conf",
            "-ORBListenEndpoints", "iiop://127.0.0.1:12351",
            "-ORBDebugLevel", "10",
            "-DCPSDebugLevel", "10",
            "-ORBLogFile", "AbstractMessageImplTest.log",
            "-DCPSConfigFile", "test.ini"
        };
        DomainParticipantFactory dpFactory = TheParticipantFactory.WithArgs(new StringSeqHolder(fakeArgs));
        assertNotNull(dpFactory);

        DomainParticipant participant = dpFactory.create_participant(1, PARTICIPANT_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
        assertNotNull(participant);

        Subscriber subscriber = participant.create_subscriber(SUBSCRIBER_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
        assertNotNull(subscriber);

        final MessagePayloadTypeSupportImpl typeSupport = new MessagePayloadTypeSupportImpl();
        assertNotNull(typeSupport);

        typeSupport.register_type(participant, "OpenDDS::JMS::MessagePayload");
        final Topic topic = participant.create_topic("OpenDDS::JMS::MessagePayload", typeSupport.get_type_name(), TOPIC_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
        assertNotNull(topic);

        Destination destination = new TopicImpl("Topic 1") {
            public Topic extractDDSTopic() {
                return topic;
            }
        };

        Publisher publisher = participant.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
        assertNotNull(publisher);

        TextMessage message = new TextMessageImpl(null);

        fakeObjects.connection = new ConnectionImpl(new ManagedConnectionImpl(new Subject(), new ConnectionRequestInfoImpl("clientID", 2, null, null, null, null, null, null)));
        fakeObjects.destination = destination;
        fakeObjects.subscriber = subscriber;
        fakeObjects.participant = participant;

        fakeObjects.publisher = publisher;
        fakeObjects.message = message;

        return fakeObjects;
    }
    private static class FakeObjects {
        public ConnectionImpl connection;
        public Destination destination;
        public Subscriber subscriber;
        public DomainParticipant participant;
        public Publisher publisher;
        public TextMessage message;
    }
}
