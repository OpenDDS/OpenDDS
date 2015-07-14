/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import javax.jms.BytesMessage;
import javax.jms.JMSException;
import javax.jms.MessageEOFException;
import javax.jms.MessageNotReadableException;
import javax.jms.MessageNotWriteableException;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import org.junit.Test;

/**
 * @author  Weiqi Gao
 */
public class BytesMessageImplTest {
    private static final float FLOAT_EPSILON = 1e-6f;
    private static final double DOUBLE_EPSILON = 1e-12;

    @Test
    public void testNewlyCeratedBytesMessage() {
        BytesMessage bytesMessage = new BytesMessageImpl(null);
        assertNotNull(bytesMessage);
    }

    @Test
    public void testWriteAndReadBytes() throws JMSException {
        BytesMessage bytesMessage = new BytesMessageImpl(null);

        bytesMessage.writeBoolean(true);
        bytesMessage.writeByte((byte)63);
        bytesMessage.writeShort((short) 1024);
        bytesMessage.writeChar('G');
        bytesMessage.writeInt(2048);
        bytesMessage.writeLong(9765625L);
        bytesMessage.writeFloat(3.14f);
        bytesMessage.writeDouble(2.718281828459045);
        bytesMessage.writeUTF("Hello OpenDDS JMS Provider");
        byte[] bytes = new byte[8];
        for (int i = 0; i < bytes.length; i++) bytes[i] = (byte) i;
        bytesMessage.writeBytes(bytes);
        bytesMessage.writeBytes(bytes, 4, 4);

        bytesMessage.reset();

        assertEquals(true, bytesMessage.readBoolean());
        assertEquals((byte) 63, bytesMessage.readByte());
        assertEquals((short) 1024, bytesMessage.readShort());
        assertEquals('G', bytesMessage.readChar());
        assertEquals(2048, bytesMessage.readInt());
        assertEquals(9765625L, bytesMessage.readLong());
        assertEquals(3.14f, bytesMessage.readFloat(), FLOAT_EPSILON);
        assertEquals(2.718281828459045, bytesMessage.readDouble(), DOUBLE_EPSILON);
        assertEquals("Hello OpenDDS JMS Provider", bytesMessage.readUTF());
        byte[] bytes2 = new byte[8];
        assertEquals(8, bytesMessage.readBytes(bytes2));
        assertArrayEquals(bytes, bytes2);
        byte[] bytes3 = new byte[4];
        assertEquals(4, bytesMessage.readBytes(bytes3));
        byte[] bytes4 = new byte[4];
        System.arraycopy(bytes, 4, bytes4, 0, 4);
        assertArrayEquals(bytes4, bytes3);

        // Now we are at the end of the bytes
        try {
            bytesMessage.readInt();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageEOFException);
        }
    }

    @Test
    public void testReadUnsignedByte() throws JMSException {
        BytesMessage bytesMessage = new BytesMessageImpl(null);
        bytesMessage.writeByte(Byte.MIN_VALUE);
        bytesMessage.writeByte((byte) -1);
        bytesMessage.writeByte((byte) 0);
        bytesMessage.writeByte((byte) 1);
        bytesMessage.writeByte(Byte.MAX_VALUE);

        bytesMessage.reset();

        int i = bytesMessage.readUnsignedByte();
        assertEquals(128, i);
        i = bytesMessage.readUnsignedByte();
        assertEquals(255, i);
        i = bytesMessage.readUnsignedByte();
        assertEquals(0, i);
        i = bytesMessage.readUnsignedByte();
        assertEquals(1, i);
        i = bytesMessage.readUnsignedByte();
        assertEquals(127, i);
    }

    @Test
    public void testReadUnsignedShort() throws JMSException {
        BytesMessage bytesMessage = new BytesMessageImpl(null);
        bytesMessage.writeShort(Short.MIN_VALUE);
        bytesMessage.writeShort((short) -1);
        bytesMessage.writeShort((short) 0);
        bytesMessage.writeShort((short) 1);
        bytesMessage.writeShort(Short.MAX_VALUE);

        bytesMessage.reset();

        int i = bytesMessage.readUnsignedShort();
        assertEquals(32768, i);
        i = bytesMessage.readUnsignedShort();
        assertEquals(65535, i);
        i = bytesMessage.readUnsignedShort();
        assertEquals(0, i);
        i = bytesMessage.readUnsignedShort();
        assertEquals(1, i);
        i = bytesMessage.readUnsignedShort();
        assertEquals(32767, i);
    }
    @Test
    public void testWriteObjects() throws JMSException {
        BytesMessage bytesMessage = new BytesMessageImpl(null);

        bytesMessage.writeObject(Boolean.TRUE);
        bytesMessage.writeObject((byte) 63);
        bytesMessage.writeObject((short) 1024);
        bytesMessage.writeObject('G');
        bytesMessage.writeObject(2048);
        bytesMessage.writeObject(9765625L);
        bytesMessage.writeObject(3.14f);
        bytesMessage.writeObject(2.718281828459045);
        bytesMessage.writeObject("Hello OpenDDS JMS Provider");
        byte[] bytes = new byte[8];
        for (int i = 0; i < bytes.length; i++) bytes[i] = (byte) i;
        bytesMessage.writeObject(bytes);

        bytesMessage.reset();

        assertEquals(true, bytesMessage.readBoolean());
        assertEquals((byte) 63, bytesMessage.readByte());
        assertEquals((short) 1024, bytesMessage.readShort());
        assertEquals('G', bytesMessage.readChar());
        assertEquals(2048, bytesMessage.readInt());
        assertEquals(9765625L, bytesMessage.readLong());
        assertEquals(3.14f, bytesMessage.readFloat(), FLOAT_EPSILON);
        assertEquals(2.718281828459045, bytesMessage.readDouble(), DOUBLE_EPSILON);
        assertEquals("Hello OpenDDS JMS Provider", bytesMessage.readUTF());
        byte[] bytes2 = new byte[8];
        assertEquals(8, bytesMessage.readBytes(bytes2));
        assertArrayEquals(bytes, bytes2);
    }

    @Test
    public void testReadBytesNormalOperation() throws JMSException {
        BytesMessage bytesMessage = new BytesMessageImpl(null);
        byte[] bytes = new byte[8];
        for (int i = 0; i < bytes.length; i++) bytes[i] = (byte) i;
        bytesMessage.writeBytes(bytes);

        bytesMessage.reset();

        byte[] bytes2 = new byte[3];
        assertEquals(3, bytesMessage.readBytes(bytes2));
        assertArrayEquals(new byte[] {(byte) 0, (byte) 1, (byte) 2}, bytes2);
        assertEquals(2, bytesMessage.readBytes(bytes2, 2));
        assertArrayEquals(new byte[] {(byte) 3, (byte) 4, (byte) 2}, bytes2);
        assertEquals(3, bytesMessage.readBytes(bytes2));
        assertArrayEquals(new byte[] {(byte) 5, (byte) 6, (byte) 7}, bytes2);
        assertEquals(-1, bytesMessage.readBytes(bytes2));
    }

    @Test
    public void testReadBytesCornerCases() throws JMSException {
        BytesMessage bytesMessage = new BytesMessageImpl(null);
        byte[] bytes = new byte[8];
        for (int i = 0; i < bytes.length; i++) bytes[i] = (byte) i;
        bytesMessage.writeBytes(bytes);

        bytesMessage.reset();

        try {
            bytesMessage.readBytes(null);
            fail("Shoule throw");
        } catch (Exception e) {
            assertTrue(e instanceof NullPointerException);
        }

        byte[] bytes2 = new byte[0];
        assertEquals(0, bytesMessage.readBytes(bytes2));
    }

    @Test
    public void testReadInWriteOnlyState() {
        BytesMessage bytesMessage = new BytesMessageImpl(null);

        cannotReadBoolean(bytesMessage);
        cannotReadByte(bytesMessage);
        cannotReadUnsignedByte(bytesMessage);
        cannotReadShort(bytesMessage);
        cannotReadUnsignedShort(bytesMessage);
        cannotReadChar(bytesMessage);
        cannotReadInt(bytesMessage);
        cannotReadLong(bytesMessage);
        cannotReadFloat(bytesMessage);
        cannotReadDouble(bytesMessage);
        cannotReadUTF(bytesMessage);
        cannotReadBytes(bytesMessage);
        cannotReadBoolean(bytesMessage);
    }

    private void cannotReadBoolean(BytesMessage bytesMessage) {
        try {
            bytesMessage.readBoolean();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
    }

    private void cannotReadByte(BytesMessage bytesMessage) {
        try {
            bytesMessage.readByte();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
    }

    private void cannotReadUnsignedByte(BytesMessage bytesMessage) {
        try {
            bytesMessage.readByte();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
    }

    private void cannotReadShort(BytesMessage bytesMessage) {
        try {
            bytesMessage.readShort();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
    }

    private void cannotReadUnsignedShort(BytesMessage bytesMessage) {
        try {
            bytesMessage.readShort();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
    }

    private void cannotReadChar(BytesMessage bytesMessage) {
        try {
            bytesMessage.readChar();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
    }

    private void cannotReadInt(BytesMessage bytesMessage) {
        try {
            bytesMessage.readInt();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
    }

    private void cannotReadLong(BytesMessage bytesMessage) {
        try {
            bytesMessage.readLong();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
    }

    private void cannotReadFloat(BytesMessage bytesMessage) {
        try {
            bytesMessage.readFloat();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
    }

    private void cannotReadDouble(BytesMessage bytesMessage) {
        try {
            bytesMessage.readDouble();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
    }

    private void cannotReadUTF(BytesMessage bytesMessage) {
        try {
            bytesMessage.readUTF();
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
    }

    private void cannotReadBytes(BytesMessage bytesMessage) {
        byte[] bytes = new byte[8];
        try {
            bytesMessage.readBytes(bytes);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
        try {
            bytesMessage.readBytes(bytes, 4);
            fail("Should throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotReadableException);
        }
    }

    @Test
    public void testWriteInReadonlyState() throws JMSException {
        BytesMessage bytesMessage = new BytesMessageImpl(null);
        bytesMessage.reset();

        cannotWriteBoolean(bytesMessage);
        cannotWriteByte(bytesMessage);
        cannotWriteShort(bytesMessage);
        cannotWriteChar(bytesMessage);
        cannotWriteInt(bytesMessage);
        cannotWriteLong(bytesMessage);
        cannotWriteFloat(bytesMessage);
        cannotWriteDouble(bytesMessage);
        cannotWriteUTF(bytesMessage);
        cannotWriteBytes(bytesMessage);
        cannotWriteObject(bytesMessage);

    }

    private void cannotWriteBoolean(BytesMessage bytesMessage) {
        try {
            bytesMessage.writeBoolean(false);
            fail("SHould throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
    }

    private void cannotWriteByte(BytesMessage bytesMessage) {
        try {
            bytesMessage.writeByte((byte) 63);
            fail("SHould throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
    }

    private void cannotWriteShort(BytesMessage bytesMessage) {
        try {
            bytesMessage.writeShort((short) 1024);
            fail("SHould throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
    }

    private void cannotWriteChar(BytesMessage bytesMessage) {
        try {
            bytesMessage.writeChar('G');
            fail("SHould throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
    }

    private void cannotWriteInt(BytesMessage bytesMessage) {
        try {
            bytesMessage.writeInt(2048);
            fail("SHould throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
    }

    private void cannotWriteLong(BytesMessage bytesMessage) {
        try {
            bytesMessage.writeLong(9765625);
            fail("SHould throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
    }

    private void cannotWriteFloat(BytesMessage bytesMessage) {
        try {
            bytesMessage.writeFloat(3.14f);
            fail("SHould throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
    }

    private void cannotWriteDouble(BytesMessage bytesMessage) {
        try {
            bytesMessage.writeDouble(2.718281828459045);
            fail("SHould throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
    }

    private void cannotWriteUTF(BytesMessage bytesMessage) {
        try {
            bytesMessage.writeUTF("Hello OpenDDS JMS Provider");
            fail("SHould throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
    }

    private void cannotWriteBytes(BytesMessage bytesMessage) {
        byte[] bytes = new byte[8];
        for(int i = 0; i < bytes.length; i++) bytes[i] = (byte) i;
        try {
            bytesMessage.writeBytes(bytes);
            fail("SHould throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
        try {
            bytesMessage.writeBytes(bytes, 4, 4);
            fail("SHould throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
    }

    private void cannotWriteObject(BytesMessage bytesMessage) {
        try {
            bytesMessage.writeObject(new Float(3.14f));
            fail("SHould throw");
        } catch (JMSException e) {
            assertTrue(e instanceof MessageNotWriteableException);
        }
    }

    @Test
    public void testClearBodyInReadOnlyState() throws JMSException {
        BytesMessage bytesMessage = new BytesMessageImpl(null);

        bytesMessage.reset();
        cannotWriteBoolean(bytesMessage);

        bytesMessage.clearBody();

        bytesMessage.writeBoolean(true);
        cannotReadBoolean(bytesMessage);
    }
}
