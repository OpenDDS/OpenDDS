/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.EOFException;
import java.io.IOException;

import javax.jms.JMSException;
import javax.jms.MessageEOFException;
import javax.jms.MessageFormatException;

import OpenDDS.JMS.MessageBody;
import OpenDDS.JMS.MessageBodyKind;

import org.opendds.jms.common.lang.Objects;

/**
 * @author  Weiqi Gao
 */
public class BytesBodyFacade {
    private final MessageBody body;
    private DataInputStream input;
    private ByteArrayOutputStream byteArrayOutputStream;
    private DataOutputStream output;

    public BytesBodyFacade(MessageBody body) {
        this.body = body;
        this.byteArrayOutputStream = new ByteArrayOutputStream();
        this.output = new DataOutputStream(byteArrayOutputStream);
    }

    public void absorbTheBytesBody() {
        if (input == null) {
            input = new DataInputStream(new ByteArrayInputStream(body.theOctetSeqBody()));
        }
    }

    public void updateTheBytesBody() {
        byte[] theNewBytesBody = byteArrayOutputStream.toByteArray();
        body.theOctetSeqBody(MessageBodyKind.BYTES_KIND, theNewBytesBody);
    }

    public long getBodyLength() {
        throw new UnsupportedOperationException();
    }

    public boolean readBoolean() throws JMSException {
        try {
            return input.readBoolean();
        } catch (EOFException e) {
            throw wrapEOFException(e);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    private JMSException wrapIOException(IOException e) {
        final JMSException jmsException = new JMSException("Internal exception");
        jmsException.setLinkedException(e);
        return jmsException;
    }

    private MessageEOFException wrapEOFException(EOFException e) {
        final MessageEOFException messageEOFException = new MessageEOFException("End of byte array reached.");
        messageEOFException.setLinkedException(e);
        return messageEOFException;
    }

    public byte readByte() throws JMSException {
        try {
            return input.readByte();
        } catch (EOFException e) {
            throw wrapEOFException(e);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public int readUnsignedByte() throws JMSException {
        try {
            return input.readUnsignedByte();
        } catch (EOFException e) {
            throw wrapEOFException(e);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public short readShort() throws JMSException {
        try {
            return input.readShort();
        } catch (EOFException e) {
            throw wrapEOFException(e);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public int readUnsignedShort() throws JMSException {
        try {
            return input.readUnsignedShort();
        } catch (EOFException e) {
            throw wrapEOFException(e);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public char readChar() throws JMSException {
        try {
            return input.readChar();
        } catch (EOFException e) {
            throw wrapEOFException(e);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public int readInt() throws JMSException {
        try {
            return input.readInt();
        } catch (EOFException e) {
            throw wrapEOFException(e);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public long readLong() throws JMSException {
        try {
            return input.readLong();
        } catch (EOFException e) {
            throw wrapEOFException(e);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public float readFloat() throws JMSException {
        try {
            return input.readFloat();
        } catch (EOFException e) {
            throw wrapEOFException(e);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public double readDouble() throws JMSException {
        try {
            return input.readDouble();
        } catch (EOFException e) {
            throw wrapEOFException(e);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public String readUTF() throws JMSException {
        try {
            return input.readUTF();
        } catch (EOFException e) {
            throw wrapEOFException(e);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public int readBytes(byte[] bytes) throws JMSException {
        try {
            return input.read(bytes);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public int readBytes(byte[] bytes, int length) throws JMSException {
        try {
            return input.read(bytes, 0, length);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public void reset() {
        input = null;
        byteArrayOutputStream.reset();
    }

    public void writeBoolean(boolean b) throws JMSException {
        try {
            output.writeBoolean(b);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public void writeByte(byte b) throws JMSException {
        try {
            output.writeByte(b);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public void writeShort(short i) throws JMSException {
        try {
            output.writeShort(i);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public void writeChar(char c) throws JMSException {
        try {
            output.writeChar(c);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public void writeInt(int i) throws JMSException {
        try {
            output.writeInt(i);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public void writeLong(long l) throws JMSException {
        try {
            output.writeLong(l);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public void writeFloat(float v) throws JMSException {
        try {
            output.writeFloat(v);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public void writeDouble(double v) throws JMSException {
        try {
            output.writeDouble(v);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public void writeUTF(String s) throws JMSException {
        try {
            output.writeUTF(s);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public void writeBytes(byte[] bytes) throws JMSException {
        try {
            output.write(bytes);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public void writeBytes(byte[] bytes, int i, int i1) throws JMSException {
        try {
            output.write(bytes, i, i1);
        } catch (IOException e) {
            throw wrapIOException(e);
        }
    }

    public void writeObject(Object o) throws JMSException {
        Objects.ensureNotNull(o);
        if (o instanceof Boolean) {
            writeBoolean((Boolean) o);
        } else if (o instanceof Byte) {
            writeByte((Byte) o);
        } else if (o instanceof Short) {
            writeShort((Short) o);
        } else if (o instanceof Character) {
            writeChar((Character) o);
        } else if (o instanceof Integer) {
            writeInt((Integer) o);
        } else if (o instanceof Long) {
            writeLong((Long) o);
        } else if (o instanceof Float) {
            writeFloat((Float) o);
        } else if (o instanceof Double) {
            writeDouble((Double) o);
        } else if (o instanceof String) {
            writeUTF((String) o);
        } else if (o instanceof byte[]) {
            writeBytes((byte[]) o);
        } else {
            throw new MessageFormatException("Invalid value type passed in for writeObject()");
        }
    }
}
