/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import javax.jms.JMSException;
import javax.jms.MessageEOFException;
import javax.jms.MessageFormatException;
import javax.jms.MessageNotWriteableException;
import javax.jms.StreamMessage;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import org.junit.Test;

import org.opendds.jms.common.lang.ByteArrays;

/**
 * @author  Weiqi Gao
 */
public class StreamMessageImplTest {
    private static final float FLOAT_EPSILON = 1e-6f;
    private static final double DOUBLE_EPSILON = 1e-12;

    @Test
    public void testNewlyCreatedStreamMessage() throws JMSException {
        StreamMessage streamMessage = new StreamMessageImpl(null);
        assertNotNull(streamMessage);
    }


    @Test
    public void testWriteAndReadStreamItems() throws JMSException {
        StreamMessage streamMessage = new StreamMessageImpl(null);

        streamMessage.writeBoolean(true);
        streamMessage.writeByte((byte) 63);
        streamMessage.writeShort((short) 1024);
        streamMessage.writeChar('G');
        streamMessage.writeInt(2048);
        streamMessage.writeLong(9765625L);
        streamMessage.writeFloat(3.14f);
        streamMessage.writeDouble(2.718281828459045);
        final String greeting = "Hello OpenDDS JMS Provider";
        streamMessage.writeString(greeting);
        byte[] bytes = new byte[8];
        for (int i = 0; i < bytes.length; i++) bytes[i] = (byte) i;
        streamMessage.writeBytes(bytes);
        streamMessage.writeBytes(bytes, 4, 4);

        streamMessage.reset();

        assertEquals(true, streamMessage.readBoolean());
        assertEquals((byte) 63, streamMessage.readByte());
        assertEquals((short) 1024, streamMessage.readShort());
        assertEquals('G', streamMessage.readChar());
        assertEquals(2048, streamMessage.readInt());
        assertEquals(9765625L, streamMessage.readLong());
        assertEquals(3.14f, streamMessage.readFloat(), FLOAT_EPSILON);
        assertEquals(2.718281828459045, streamMessage.readDouble(), DOUBLE_EPSILON);
        assertEquals(greeting, streamMessage.readString());

        byte[] bytes2 = new byte[8];
        final int i = streamMessage.readBytes(bytes2);
        assertEquals(8, i);
        assertArrayEquals(bytes, bytes2);
        assertEquals(-1, streamMessage.readBytes(bytes2));

        byte[] bytes3 = new byte[4];
        final int i2 = streamMessage.readBytes(bytes3);
        assertEquals(4, i2);
        byte[] subBytes = ByteArrays.extractSubBytes(bytes, 4, 4);
        assertArrayEquals(subBytes, bytes3);
        assertEquals(-1, streamMessage.readBytes(bytes3));
    }

    @Test
    public void testWriteObjects() throws JMSException {
        StreamMessage streamMessage = new StreamMessageImpl(null);

        streamMessage.writeObject(Boolean.TRUE);
        streamMessage.writeObject((byte) 63);
        streamMessage.writeObject((short) 1024);
        streamMessage.writeObject('G');
        streamMessage.writeObject(2048);
        streamMessage.writeObject(9765625L);
        streamMessage.writeObject(3.14f);
        streamMessage.writeObject(2.718281828459045);
        final String greeting = "Hello OpenDDS JMS Provider";
        streamMessage.writeObject(greeting);
        byte[] bytes = new byte[8];
        for (int i = 0; i < bytes.length; i++) bytes[i] = (byte) i;
        streamMessage.writeObject(bytes);

        streamMessage.reset();

        assertEquals(true, streamMessage.readBoolean());
        assertEquals((byte) 63, streamMessage.readByte());
        assertEquals((short) 1024, streamMessage.readShort());
        assertEquals('G', streamMessage.readChar());
        assertEquals(2048, streamMessage.readInt());
        assertEquals(9765625L, streamMessage.readLong());
        assertEquals(3.14f, streamMessage.readFloat(), FLOAT_EPSILON);
        assertEquals(2.718281828459045, streamMessage.readDouble(), DOUBLE_EPSILON);
        assertEquals(greeting, streamMessage.readString());

        byte[] bytes2 = new byte[8];
        final int i = streamMessage.readBytes(bytes2);
        assertEquals(8, i);
        assertArrayEquals(bytes, bytes2);
        assertEquals(-1, streamMessage.readBytes(bytes2));
    }

    @Test
    public void testReadObjects() throws JMSException {
        StreamMessage streamMessage = new StreamMessageImpl(null);
        populateMessage(streamMessage);
        streamMessage.reset();

        assertEquals(Boolean.TRUE, streamMessage.readObject());
        assertEquals((byte) 63, streamMessage.readObject());
        assertEquals((short) 1024, streamMessage.readObject());
        assertEquals('G', streamMessage.readObject());
        assertEquals(2048, streamMessage.readObject());
        assertEquals(9765625L, streamMessage.readObject());
        assertEquals(3.14f, streamMessage.readObject());
        assertEquals(2.718281828459045, streamMessage.readObject());
        assertEquals("Hello OpenDDS JMS Provider", streamMessage.readObject());
        byte[] bytes1 = new byte[8];
        for (int i = 0; i < bytes1.length; i++) bytes1[i] = (byte) i;
        assertArrayEquals(bytes1, (byte[]) streamMessage.readObject());
    }

    @Test
    public void testReadBytesNormalOperation() throws JMSException {
        StreamMessage streamMessage = new StreamMessageImpl(null);
        populateMessage(streamMessage);

        // Peel off some stream items so that we can get at the byte arrays
        streamMessage.reset();
        for (int i = 0; i < 9; i++) {
            streamMessage.readObject();
        }

        byte[] bytes1 = new byte[3];
        // item 9 has 8 bytes
        assertEquals(3, streamMessage.readBytes(bytes1));
        assertArrayEquals(new byte[]{(byte) 0, (byte) 1, (byte) 2}, bytes1);
        assertEquals(3, streamMessage.readBytes(bytes1));
        assertArrayEquals(new byte[]{(byte) 3, (byte) 4, (byte) 5}, bytes1);
        assertEquals(2, streamMessage.readBytes(bytes1));
        assertArrayEquals(new byte[]{(byte) 6, (byte) 7, (byte) 5}, bytes1);  // third element not overwritten
        // item 10 has 4 bytes
        assertEquals(3, streamMessage.readBytes(bytes1));
        assertArrayEquals(new byte[]{(byte) 4, (byte) 5, (byte) 6}, bytes1);
        assertEquals(1, streamMessage.readBytes(bytes1));
        assertArrayEquals(new byte[]{(byte) 7, (byte) 5, (byte) 6}, bytes1);  // second and third element not overwritten
    }

    void populateMessage(StreamMessage streamMessage) throws JMSException {
        streamMessage.writeBoolean(true);
        streamMessage.writeByte((byte) 63);
        streamMessage.writeShort((short) 1024);
        streamMessage.writeChar('G');
        streamMessage.writeInt(2048);
        streamMessage.writeLong(9765625L);
        streamMessage.writeFloat(3.14f);
        streamMessage.writeDouble(2.718281828459045);
        final String greeting = "Hello OpenDDS JMS Provider";
        streamMessage.writeString(greeting);
        byte[] bytes = new byte[8];
        for (int i = 0; i < bytes.length; i++) bytes[i] = (byte) i;
        streamMessage.writeBytes(bytes);
        streamMessage.writeBytes(bytes, 4, 4);
    }

    @Test
    public void testReadBytesCornerCases() throws JMSException {
        StreamMessage streamMessage = new StreamMessageImpl(null);
        populateMessage(streamMessage);

        // Peel off some stream items so that we can get at the byte arrays
        streamMessage.reset();
        for (int i = 0; i < 9; i++) {
            streamMessage.readObject();
        }

        byte[] bytes = null;
        assertEquals(-1, streamMessage.readBytes(bytes));

        bytes = new byte[0];
        assertEquals(0, streamMessage.readBytes(bytes));

        // partially read an byte array...
        bytes = new byte[4];
        assertEquals(4, streamMessage.readBytes(bytes));

        // ... prevents reading of any other type
        cannotReadBoolean(streamMessage);
        cannotReadByte(streamMessage);
        cannotReadShort(streamMessage);
        cannotReadChar(streamMessage);
        cannotReadInt(streamMessage);
        cannotReadLong(streamMessage);
        cannotReadFloat(streamMessage);
        cannotReadDouble(streamMessage);
        cannotReadString(streamMessage);
        cannotReadObject(streamMessage);

        // read again...
        assertEquals(4, streamMessage.readBytes(bytes));

        // ... we still can't read any other type
        cannotReadObject(streamMessage);

        // must make another read
        assertEquals(-1, streamMessage.readBytes(bytes));
    }

    @Test
    public void testWriteStreamItemInNotWritableState() throws JMSException {
        StreamMessage streamMessage = new StreamMessageImpl(null);
        streamMessage.reset();

        try {
            streamMessage.writeBoolean(true);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            streamMessage.writeByte((byte) 63);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            streamMessage.writeShort((short) 1024);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            streamMessage.writeChar('G');
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            streamMessage.writeInt(1024);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            streamMessage.writeLong(9765625L);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            streamMessage.writeFloat(3.14f);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            streamMessage.writeDouble(2.718281828459045);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            streamMessage.writeString("Hello OpenDDS JMS Provider");
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            byte[] bytes = new byte[8];
            for (int i = 0; i < bytes.length; i++) bytes[i] = (byte) i;
            streamMessage.writeBytes(bytes);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            byte[] bytes = new byte[8];
            for (int i = 0; i < bytes.length; i++) bytes[i] = (byte) i;
            streamMessage.writeBytes(bytes, 4, 4);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }

        try {
            streamMessage.writeObject(new Long(9765625L));
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
    }

    @Test
    public void testRereadStreamItems() throws JMSException {
        StreamMessage streamMessage = new StreamMessageImpl(null);
        populateMessage(streamMessage);
        streamMessage.reset();

        // The boolean
        cannotReadByte(streamMessage);
        cannotReadShort(streamMessage);
        cannotReadChar(streamMessage);
        cannotReadInt(streamMessage);
        cannotReadLong(streamMessage);
        cannotReadFloat(streamMessage);
        cannotReadDouble(streamMessage);
        cannotReadBytes(streamMessage);
        assertEquals("true", streamMessage.readString());

        // The byte
        cannotReadBoolean(streamMessage);
        cannotReadChar(streamMessage);
        cannotReadFloat(streamMessage);
        cannotReadDouble(streamMessage);
        cannotReadBytes(streamMessage);
        assertEquals("63", streamMessage.readString());

        // The short
        cannotReadBoolean(streamMessage);
        cannotReadByte(streamMessage);
        cannotReadChar(streamMessage);
        cannotReadFloat(streamMessage);
        cannotReadDouble(streamMessage);
        cannotReadBytes(streamMessage);
        assertEquals("1024", streamMessage.readString());

        // The char
        cannotReadBoolean(streamMessage);
        cannotReadByte(streamMessage);
        cannotReadShort(streamMessage);
        cannotReadInt(streamMessage);
        cannotReadLong(streamMessage);
        cannotReadFloat(streamMessage);
        cannotReadDouble(streamMessage);
        cannotReadBytes(streamMessage);
        assertEquals("G", streamMessage.readString());

        // The int
        cannotReadBoolean(streamMessage);
        cannotReadByte(streamMessage);
        cannotReadShort(streamMessage);
        cannotReadChar(streamMessage);
        cannotReadFloat(streamMessage);
        cannotReadDouble(streamMessage);
        cannotReadBytes(streamMessage);
        assertEquals("2048", streamMessage.readString());

        // The long
        cannotReadBoolean(streamMessage);
        cannotReadByte(streamMessage);
        cannotReadShort(streamMessage);
        cannotReadChar(streamMessage);
        cannotReadInt(streamMessage);
        cannotReadFloat(streamMessage);
        cannotReadDouble(streamMessage);
        cannotReadBytes(streamMessage);
        assertEquals("9765625", streamMessage.readString());

        // The float
        cannotReadBoolean(streamMessage);
        cannotReadByte(streamMessage);
        cannotReadShort(streamMessage);
        cannotReadChar(streamMessage);
        cannotReadInt(streamMessage);
        cannotReadLong(streamMessage);
        cannotReadBytes(streamMessage);
        assertEquals("3.14", streamMessage.readString());

        // The ddouble
        cannotReadBoolean(streamMessage);
        cannotReadByte(streamMessage);
        cannotReadShort(streamMessage);
        cannotReadChar(streamMessage);
        cannotReadInt(streamMessage);
        cannotReadLong(streamMessage);
        cannotReadFloat(streamMessage);
        cannotReadBytes(streamMessage);
        assertEquals("2.718281828459045", streamMessage.readString());

        // The string
        cannotReadChar(streamMessage);
        cannotReadBytes(streamMessage);
        assertEquals("Hello OpenDDS JMS Provider", streamMessage.readString());

        // The byte array
        cannotReadBoolean(streamMessage);
        cannotReadByte(streamMessage);
        cannotReadShort(streamMessage);
        cannotReadChar(streamMessage);
        cannotReadInt(streamMessage);
        cannotReadLong(streamMessage);
        cannotReadFloat(streamMessage);
        cannotReadDouble(streamMessage);
        cannotReadString(streamMessage);
        byte[] bytes = new byte[9];
        assertEquals(8, streamMessage.readBytes(bytes));
    }

    private void cannotReadObject(StreamMessage streamMessage) {
        try {
            streamMessage.readObject();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageFormatException);
        }
    }

    private void cannotReadInt(StreamMessage streamMessage) {
        try {
            streamMessage.readInt();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageFormatException);
        }
    }

    private void cannotReadString(StreamMessage streamMessage) {
        try {
            streamMessage.readString();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageFormatException);
        }
    }

    private void cannotReadBoolean(StreamMessage streamMessage) {
        try {
            streamMessage.readBoolean();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageFormatException);
        }
    }

    private void cannotReadBytes(StreamMessage streamMessage) {
        try {
            byte[] bytes = new byte[8];
            streamMessage.readBytes(bytes);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageFormatException);
        }
    }

    private void cannotReadDouble(StreamMessage streamMessage) {
        try {
            streamMessage.readDouble();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageFormatException);
        }
    }

    private void cannotReadFloat(StreamMessage streamMessage) {
        try {
            streamMessage.readFloat();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageFormatException);
        }
    }

    private void cannotReadLong(StreamMessage streamMessage) {
        try {
            streamMessage.readLong();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageFormatException);
        }
    }

    private void cannotReadChar(StreamMessage streamMessage) {
        try {
            streamMessage.readChar();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageFormatException);
        }
    }

    private void cannotReadShort(StreamMessage streamMessage) {
        try {
            streamMessage.readShort();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageFormatException);
        }
    }

    private void cannotReadByte(StreamMessage streamMessage) {
        try {
            streamMessage.readByte();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageFormatException);
        }
    }

    @Test
    public void testClearBody() throws JMSException {
        StreamMessage streamMessage = new StreamMessageImpl(null);
        populateMessage(streamMessage);

        streamMessage.clearBody();
        streamMessage.reset();

        try {
            streamMessage.readObject();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageEOFException);
        }
    }

    @Test
    public void testReset() throws JMSException {
        StreamMessage streamMessage = new StreamMessageImpl(null);
        populateMessage(streamMessage);

        streamMessage.reset();

        try {
            streamMessage.writeObject(new Double(2.718281828459045));
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
    }
}
