/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.util.Enumeration;
import java.util.HashSet;
import java.util.Set;

import javax.jms.JMSException;
import javax.jms.MapMessage;
import javax.jms.MessageFormatException;
import javax.jms.MessageNotWriteableException;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import org.junit.Test;

/**
 * @author  Weiqi Gao
 */
public class MapMessageImplTest {
    private static final float FLOAT_EPSILON = 1e-6f;
    private static final double DOUBLE_EPSILON = 1e-12;

    @Test
    public void testNewlyCreatedMapMessage() {
        MapMessage mapMessage = new MapMessageImpl(null);
        assertNotNull(mapMessage);
    }

    @Test
    public void testSetAndGetMapItems() throws JMSException {
        MapMessage mapMessage = new MapMessageImpl(null);

        mapMessage.setBoolean("boolean", true);
        assertEquals(true, mapMessage.getBoolean("boolean"));

        mapMessage.setBoolean("boolean", false);
        assertEquals(false, mapMessage.getBoolean("boolean"));

        mapMessage.setByte("byte", (byte) 63);
        assertEquals((byte) 63, mapMessage.getByte("byte"));

        mapMessage.setShort("short", (short) 1024);
        assertEquals((short) 1024, mapMessage.getShort("short"));

        mapMessage.setChar("char", 'G');
        assertEquals('G', mapMessage.getChar("char"));

        mapMessage.setInt("int", 2048);
        assertEquals(2048, mapMessage.getInt("int"));

        mapMessage.setLong("long", 9765625);
        assertEquals(9765625, mapMessage.getLong("long"));

        mapMessage.setFloat("float", 3.14f);
        assertEquals(3.14, mapMessage.getFloat("float"), FLOAT_EPSILON);

        mapMessage.setDouble("double", 2.718281828459045);
        assertEquals(2.718281828459045, mapMessage.getDouble("double"), DOUBLE_EPSILON);

        final String greeting = "Hello OpenDDS JMS Provider";
        mapMessage.setString("string", greeting);
        assertEquals(greeting, mapMessage.getString("string"));

        byte[] bytes = new byte[8];
        for (int i = 0; i < bytes.length; i++) {
            bytes[i] = (byte) i;
        }
        mapMessage.setBytes("bytes", bytes);
        assertArrayEquals(bytes, mapMessage.getBytes("bytes"));

        mapMessage.setBytes("bytes", bytes, 4, 4);
        byte[] bytes2 = new byte[4];
        System.arraycopy(bytes, 4, bytes2, 0, 4);
        assertArrayEquals(bytes2, mapMessage.getBytes("bytes"));

        Object o = new Long(9765625);
        mapMessage.setObject("object", o);
        assertEquals(o, mapMessage.getObject("object"));
    }

    @Test
    public void testUpdatingMapItems() throws JMSException {
        MapMessage mapMessage = new MapMessageImpl(null);
        populateMapItems(mapMessage);

        mapMessage.setBoolean("boolean", false);
        assertEquals(false, mapMessage.getBoolean("boolean"));

        mapMessage.setByte("byte", (byte) 127);
        assertEquals((byte) 127, mapMessage.getByte("byte"));

        mapMessage.setShort("short", (short) 10240);
        assertEquals((short) 10240, mapMessage.getShort("short"));

        mapMessage.setChar("char", 'W');
        assertEquals('W', mapMessage.getChar("char"));

        mapMessage.setInt("int", 20480);
        assertEquals(20480, mapMessage.getInt("int"));

        mapMessage.setLong("long", 9765626L);
        assertEquals(9765626L, mapMessage.getLong("long"));

        mapMessage.setFloat("float", 6.28f);
        assertEquals(6.28f, mapMessage.getFloat("float"), FLOAT_EPSILON);

        mapMessage.setDouble("double", 27.18281828459045);
        assertEquals(27.18281828459045, mapMessage.getDouble("double"), DOUBLE_EPSILON);

        final String greeting = "Goodbye OpenDDS JMS Provider";
        mapMessage.setString("string", greeting);
        assertEquals(greeting, mapMessage.getString("string"));

        byte[] bytes = new byte[8];
        for (int i = 0; i < bytes.length; i++) {
            bytes[i] = (byte) (8 - i);
        }
        mapMessage.setBytes("bytes", bytes);
        assertArrayEquals(bytes, mapMessage.getBytes("bytes"));

        mapMessage.setBytes("bytes", bytes, 0, 6);
        byte[] bytes2 = new byte[6];
        System.arraycopy(bytes, 0, bytes2, 0, 6);
        assertArrayEquals(bytes2, mapMessage.getBytes("bytes"));

        Object o = new Long(9765626L);
        mapMessage.setObject("object", o);
        assertEquals(o, mapMessage.getObject("object"));
    }

    private void populateMapItems(MapMessage mapMessage) throws JMSException {
        mapMessage.setBoolean("boolean", true);
        mapMessage.setByte("byte", (byte) 63);
        mapMessage.setShort("short", (short) 1024);
        mapMessage.setChar("char", 'G');
        mapMessage.setInt("int", 2048);
        mapMessage.setLong("long", 9765625);
        mapMessage.setFloat("float", 3.14f);
        mapMessage.setDouble("double", 2.718281828459045);
        final String greeting = "Hello OpenDDS JMS Provider";
        mapMessage.setString("string", greeting);
        byte[] bytes = new byte[8];
        for (int i = 0; i < bytes.length; i++) bytes[i] = (byte) i;
        mapMessage.setBytes("bytes", bytes);
        mapMessage.setObject("object", new Long(9765625));
    }

    @Test
    public void testSetMapItemsInNotWritableState() throws JMSException {
        MapMessageImpl mapMessage = new MapMessageImpl(null);
        mapMessage.setBodyState(new MessageStateBodyNonWritable(mapMessage));

        try {
            mapMessage.setBoolean("boolean", true);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue( e instanceof MessageNotWriteableException);
        }

        try {
            mapMessage.setByte("byte", (byte) 63);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue( e instanceof MessageNotWriteableException);
        }

        try {
            mapMessage.setShort("short", (short) 1024);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue( e instanceof MessageNotWriteableException);
        }

        try {
            mapMessage.setChar("char", 'G');
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue( e instanceof MessageNotWriteableException);
        }

        try {
            mapMessage.setInt("int", 2048);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue( e instanceof MessageNotWriteableException);
        }

        try {
            mapMessage.setLong("long", 9765625);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue( e instanceof MessageNotWriteableException);
        }

        try {
            mapMessage.setFloat("float", 3.14f);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue( e instanceof MessageNotWriteableException);
        }

        try {
            mapMessage.setDouble("double", 2.718281828459045);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue( e instanceof MessageNotWriteableException);
        }

        try {
            final String greeting = "Hello OpenDDS JMS Provider";
            mapMessage.setString("string", greeting);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue( e instanceof MessageNotWriteableException);
        }

        byte[] bytes = new byte[8];
        for (int i = 0; i < bytes.length; i++) bytes[i] = (byte) i;

        try {
            mapMessage.setBytes("bytes", bytes);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue( e instanceof MessageNotWriteableException);
        }

        try {
            mapMessage.setBytes("bytes", bytes, 4, 4);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue( e instanceof MessageNotWriteableException);
        }

        try {
            Object o = new Long(9765625);
            mapMessage.setObject("object", o);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
    }

    // JMS 1.1, 3.11.3
    @Test
    public void testStreamItemConversion() throws JMSException {
        MapMessage mapMessage = new MapMessageImpl(null);
        populateMapItems(mapMessage);

        // Successful conversions
        assertEquals("true", mapMessage.getString("boolean"));

        assertEquals((short) 63, mapMessage.getShort("byte"));
        assertEquals(63, mapMessage.getInt("byte"));
        assertEquals(63L, mapMessage.getLong("byte"));
        assertEquals("63", mapMessage.getString("byte"));

        assertEquals(1024, mapMessage.getInt("short"));
        assertEquals(1024L, mapMessage.getLong("short"));
        assertEquals("1024", mapMessage.getString("short"));

        assertEquals("G", mapMessage.getString("char"));

        assertEquals(2048L, mapMessage.getLong("int"));
        assertEquals("2048", mapMessage.getString("int"));

        assertEquals("9765625", mapMessage.getString("long"));

        assertEquals(3.14D, mapMessage.getDouble("float"), FLOAT_EPSILON);
        assertEquals("3.14", mapMessage.getString("float"));

        assertEquals("2.718281828459045", mapMessage.getString("double"));

        mapMessage.setString("string", "true");
        assertEquals(true, mapMessage.getBoolean("string"));

        mapMessage.setString("string", "63");
        assertEquals((byte) 63, mapMessage.getByte("string"));

        mapMessage.setString("string", "1024");
        assertEquals((short) 1024, mapMessage.getShort("string"));

        mapMessage.setString("string", "2048");
        assertEquals(2048, mapMessage.getInt("string"));

        mapMessage.setString("string", "9765625");
        assertEquals(9765625L, mapMessage.getLong("string"));

        mapMessage.setString("string", "3.14");
        assertEquals(3.14f, mapMessage.getFloat("string"), FLOAT_EPSILON);

        mapMessage.setString("string", "2.718281828459045");
        assertEquals(2.718281828459045, mapMessage.getDouble("string"), DOUBLE_EPSILON);

        // Illegal conversions
        cannotConvertToByte(mapMessage, "boolean");
        cannotConvertToShort(mapMessage, "boolean");
        cannotConvertToChar(mapMessage, "boolean");
        cannotConvertToInt(mapMessage, "boolean");
        cannotConvertToLong(mapMessage, "boolean");
        cannotConvertToFloat(mapMessage, "boolean");
        cannotConvertToDouble(mapMessage, "boolean");
        cannotConvertToByteArray(mapMessage, "boolean");

        cannotConvertToBoolean(mapMessage, "byte");
        cannotConvertToChar(mapMessage, "byte");
        cannotConvertToFloat(mapMessage, "byte");
        cannotConvertToDouble(mapMessage, "byte");
        cannotConvertToByteArray(mapMessage, "byte");

        cannotConvertToBoolean(mapMessage, "short");
        cannotConvertToByte(mapMessage, "short");
        cannotConvertToChar(mapMessage, "short");
        cannotConvertToFloat(mapMessage, "short");
        cannotConvertToDouble(mapMessage, "short");
        cannotConvertToByteArray(mapMessage, "short");

        cannotConvertToBoolean(mapMessage, "char");
        cannotConvertToByte(mapMessage, "char");
        cannotConvertToShort(mapMessage, "char");
        cannotConvertToInt(mapMessage, "char");
        cannotConvertToLong(mapMessage, "char");
        cannotConvertToFloat(mapMessage, "char");
        cannotConvertToDouble(mapMessage, "char");
        cannotConvertToByteArray(mapMessage, "char");

        cannotConvertToBoolean(mapMessage, "int");
        cannotConvertToByte(mapMessage, "int");
        cannotConvertToShort(mapMessage, "int");
        cannotConvertToChar(mapMessage, "int");
        cannotConvertToFloat(mapMessage, "int");
        cannotConvertToDouble(mapMessage, "int");
        cannotConvertToByteArray(mapMessage, "int");

        cannotConvertToBoolean(mapMessage, "long");
        cannotConvertToByte(mapMessage, "long");
        cannotConvertToShort(mapMessage, "long");
        cannotConvertToChar(mapMessage, "long");
        cannotConvertToInt(mapMessage, "long");
        cannotConvertToFloat(mapMessage, "long");
        cannotConvertToDouble(mapMessage, "long");
        cannotConvertToByteArray(mapMessage, "long");

        cannotConvertToBoolean(mapMessage, "float");
        cannotConvertToByte(mapMessage, "float");
        cannotConvertToShort(mapMessage, "float");
        cannotConvertToChar(mapMessage, "float");
        cannotConvertToInt(mapMessage, "float");
        cannotConvertToLong(mapMessage, "float");
        cannotConvertToByteArray(mapMessage, "float");

        cannotConvertToBoolean(mapMessage, "double");
        cannotConvertToByte(mapMessage, "double");
        cannotConvertToShort(mapMessage, "double");
        cannotConvertToChar(mapMessage, "double");
        cannotConvertToInt(mapMessage, "double");
        cannotConvertToLong(mapMessage, "double");
        cannotConvertToFloat(mapMessage, "double");
        cannotConvertToByteArray(mapMessage, "double");

        cannotConvertToByteArray(mapMessage, "string");

        cannotConvertToBoolean(mapMessage, "bytes");
        cannotConvertToByte(mapMessage, "bytes");
        cannotConvertToShort(mapMessage, "bytes");
        cannotConvertToChar(mapMessage, "bytes");
        cannotConvertToInt(mapMessage, "bytes");
        cannotConvertToLong(mapMessage, "bytes");
        cannotConvertToFloat(mapMessage, "bytes");
        cannotConvertToDouble(mapMessage, "bytes");
        cannotConvertToString(mapMessage, "bytes");
    }

    private void cannotConvertToBoolean(MapMessage mapMessage, String s) throws JMSException {
        try {
            mapMessage.getBoolean(s);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Cannot convert stream item to boolean", e.getMessage());
        }
    }

    private void cannotConvertToByte(MapMessage mapMessage, String s) throws JMSException {
        try {
            mapMessage.getByte(s);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Cannot convert stream item to byte", e.getMessage());
        }
    }

    private void cannotConvertToShort(MapMessage mapMessage, String s) throws JMSException {
        try {
            mapMessage.getShort(s);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Cannot convert stream item to short", e.getMessage());
        }
    }

    private void cannotConvertToChar(MapMessage mapMessage, String s) throws JMSException {
        try {
            mapMessage.getChar(s);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Cannot convert stream item to char", e.getMessage());
        }
    }

    private void cannotConvertToInt(MapMessage mapMessage, String s) throws JMSException {
        try {
            mapMessage.getInt(s);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Cannot convert stream item to int", e.getMessage());
        }
    }

    private void cannotConvertToLong(MapMessage mapMessage, String s) throws JMSException {
        try {
            mapMessage.getLong(s);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Cannot convert stream item to long", e.getMessage());
        }
    }

    private void cannotConvertToFloat(MapMessage mapMessage, String s) throws JMSException {
        try {
            mapMessage.getFloat(s);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Cannot convert stream item to float", e.getMessage());
        }
    }

    private void cannotConvertToDouble(MapMessage mapMessage, String s) throws JMSException {
        try {
            mapMessage.getDouble(s);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Cannot convert stream item to double", e.getMessage());
        }
    }

    private void cannotConvertToString(MapMessage mapMessage, String s) throws JMSException {
        try {
            mapMessage.getString(s);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Cannot convert stream item to string", e.getMessage());
        }
    }

    private void cannotConvertToByteArray(MapMessage mapMessage, String s) throws JMSException {
        try {
            mapMessage.getBytes(s);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Cannot convert stream item to bytes", e.getMessage());
        }
    }

    @Test
    public void testStreamItemConversionFromNull() throws JMSException {
        MapMessage mapMessage = new MapMessageImpl(null);
        mapMessage.setString("string", null);

        assertEquals(false, mapMessage.getBoolean("string"));

        try {
            mapMessage.getByte("string");
            fail("Should throw");
        } catch (NumberFormatException e) {
            assertEquals("null", e.getMessage());
        }

        try {
            mapMessage.getShort("string");
            fail("Should throw");
        } catch (NumberFormatException e) {
            assertEquals("null", e.getMessage());
        }

        try {
            mapMessage.getInt("string");
            fail("Should throw");
        } catch (NumberFormatException e) {
            assertEquals("null", e.getMessage());
        }

        try {
            mapMessage.getLong("string");
            fail("Should throw");
        } catch (NumberFormatException e) {
            assertEquals("null", e.getMessage());
        }

        try {
            mapMessage.getFloat("string");
            fail("Should throw");
        } catch (NullPointerException e) {
            assertNull(e.getMessage());
        }

        try {
            mapMessage.getDouble("string");
            fail("Should throw");
        } catch (NullPointerException e) {
            assertNull(e.getMessage());
        }
    }

    @Test
    public void testSetObject() throws JMSException {
        MapMessage mapMessage = new MapMessageImpl(null);

        mapMessage.setObject("object", Boolean.TRUE);
        assertEquals(true, mapMessage.getBoolean("object"));


        mapMessage.setObject("object", Byte.valueOf("63"));
        assertEquals((byte)63, mapMessage.getByte("object"));

        mapMessage.setObject("object", Short.valueOf((short) 1024));
        assertEquals((short) 1024, mapMessage.getShort("object"));

        mapMessage.setObject("object", Character.valueOf('G'));
        assertEquals('G', mapMessage.getChar("object"));

        mapMessage.setObject("object", Integer.valueOf(2048));
        assertEquals(2048, mapMessage.getInt("object"));

        mapMessage.setObject("object", Long.valueOf(9765625));
        assertEquals(9765625L, mapMessage.getLong("object"));

        mapMessage.setObject("object", Float.valueOf(3.14f));
        assertEquals(3.14f, mapMessage.getFloat("object"), FLOAT_EPSILON);

        mapMessage.setObject("object", Double.valueOf(2.718281828459045));
        assertEquals(2.718281828459045, mapMessage.getDouble("object"), DOUBLE_EPSILON);

        final String greeting = "Hello OpenDDS JMS Provider";
        mapMessage.setObject("object", greeting);
        assertEquals(greeting, mapMessage.getString("object"));

        final byte[] bytes = new byte[8];
        for (int i = 0; i < bytes.length; i++) bytes[i] = (byte) i;
        mapMessage.setObject("object", bytes);
        assertArrayEquals(bytes, mapMessage.getBytes("object"));

        try {
            mapMessage.setObject("object", null);
            fail("Should throw");
        } catch (MessageFormatException e) {
            assertEquals("Invalid value passed in for setObject()", e.getMessage());
        }
    }

    @Test
    public void testGetObject() throws JMSException {
        MapMessage mapMessage = new MapMessageImpl(null);
        populateMapItems(mapMessage);

        assertEquals(Boolean.TRUE, mapMessage.getObject("boolean"));
        assertEquals(Byte.valueOf((byte)63), mapMessage.getObject("byte"));
        assertEquals(Short.valueOf((short) 1024), mapMessage.getObject("short"));
        assertEquals(Character.valueOf('G'), mapMessage.getObject("char"));
        assertEquals(Integer.valueOf(2048), mapMessage.getObject("int"));
        assertEquals(Long.valueOf(9765625), mapMessage.getObject("long"));
        assertEquals(Float.valueOf(3.14f), mapMessage.getObject("float"));
        assertEquals(Double.valueOf(2.718281828459045), mapMessage.getObject("double"));
        assertEquals("Hello OpenDDS JMS Provider", mapMessage.getObject("string"));

        byte[] bytes = new byte[8];
        for (int i = 0; i < bytes.length; i++) bytes[i] = (byte) i;
        final Object o = mapMessage.getObject("bytes");
        assertTrue(o instanceof byte[]);
        assertArrayEquals(bytes, (byte[]) o);

        assertNull(mapMessage.getObjectProperty("nonexistent"));
    }

    @Test
    public void testGettingNonExistentMapItems() throws JMSException {
        MapMessage mapMessage = new MapMessageImpl(null);

        assertEquals(false, mapMessage.getBoolean("nonexistent"));

        try {
            mapMessage.getByte("nonexistent");
            fail("Should throw");
        } catch (NumberFormatException e) {
            assertEquals("null", e.getMessage());
        }

        try {
            mapMessage.getShort("nonexistent");
            fail("Should throw");
        } catch (NumberFormatException e) {
            assertEquals("null", e.getMessage());
        }

        try {
            mapMessage.getChar("nonexistent");
            fail("Should throw");
        } catch (NullPointerException e) {
            assertNull(e.getMessage());
        }

        try {
            mapMessage.getInt("nonexistent");
            fail("Should throw");
        } catch (NumberFormatException e) {
            assertEquals("null", e.getMessage());
        }

        try {
            mapMessage.getLong("nonexistent");
            fail("Should throw");
        } catch (NumberFormatException e) {
            assertEquals("null", e.getMessage());
        }

        try {
            mapMessage.getFloat("nonexistent");
            fail("Should throw");
        } catch (NullPointerException e) {
            assertNull(e.getMessage());
        }

        try {
            mapMessage.getDouble("nonexistent");
            fail("Should throw");
        } catch (NullPointerException e) {
            assertNull(e.getMessage());
        }

        assertNull(mapMessage.getString("nonexistent"));
        assertNull(mapMessage.getBytes("nonexistent"));
        assertNull(mapMessage.getObject("nonexistent"));
    }

    @Test
    public void testGetMapNames() throws JMSException {
        MapMessage mapMessage = new MapMessageImpl(null);
        populateMapItems(mapMessage);

        final Enumeration mapNames = mapMessage.getMapNames();
        Set<String> names = new HashSet<String>();
        while (mapNames.hasMoreElements()) {
            final String name = (String) mapNames.nextElement();
            names.add(name);
        }

        assertEquals(11, names.size());
        assertTrue(names.contains("boolean"));
        assertTrue(names.contains("byte"));
        assertTrue(names.contains("short"));
        assertTrue(names.contains("char"));
        assertTrue(names.contains("int"));
        assertTrue(names.contains("long"));
        assertTrue(names.contains("float"));
        assertTrue(names.contains("double"));
        assertTrue(names.contains("string"));
        assertTrue(names.contains("bytes"));
        assertTrue(names.contains("object"));

    }

    @Test
    public void testItemExists() throws JMSException {
        MapMessage mapMessage = new MapMessageImpl(null);
        populateMapItems(mapMessage);

        assertTrue(mapMessage.itemExists("boolean"));
        assertTrue(mapMessage.itemExists("byte"));
        assertTrue(mapMessage.itemExists("short"));
        assertTrue(mapMessage.itemExists("char"));
        assertTrue(mapMessage.itemExists("int"));
        assertTrue(mapMessage.itemExists("long"));
        assertTrue(mapMessage.itemExists("float"));
        assertTrue(mapMessage.itemExists("double"));
        assertTrue(mapMessage.itemExists("string"));
        assertTrue(mapMessage.itemExists("bytes"));
        assertTrue(mapMessage.itemExists("object"));
    }

    @Test
    public void testClearBody() throws JMSException {
        MapMessage mapMessage = new MapMessageImpl(null);
        populateMapItems(mapMessage);
        assertTrue(mapMessage.getMapNames().hasMoreElements());

        mapMessage.clearBody();
        assertFalse(mapMessage.getMapNames().hasMoreElements());
    }

    @Test
    public void testClearBodyInNonWritableState() throws JMSException {
        MapMessageImpl mapMessage = new MapMessageImpl(null);
        populateMapItems(mapMessage);
        assertTrue(mapMessage.getMapNames().hasMoreElements());
        mapMessage.setBodyState(new MessageStateBodyNonWritable(mapMessage));

        mapMessage.clearBody();
        assertFalse(mapMessage.getMapNames().hasMoreElements());
    }
}
