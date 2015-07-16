/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import javax.jms.BytesMessage;
import javax.jms.JMSException;

import OpenDDS.JMS.MessageBodyKind;
import OpenDDS.JMS.MessagePayload;

/**
 * @author  Weiqi Gao
 */
public class BytesMessageImpl extends AbstractMessageImpl implements BytesMessage {
    protected BytesBodyFacade bytesBody;

    public BytesMessageImpl(SessionImpl sessionImpl) {
        super(sessionImpl);
        initBody();
    }

    public BytesMessageImpl(MessagePayload messagePayload, int handle, SessionImpl sessionImpl) {
        super(messagePayload, handle, sessionImpl);
        setBodyState(new MessageStateBodyReadOnly(this));
        bytesBody.reset();
    }

    private void initBody() {
        payload.theBody.theOctetSeqBody(MessageBodyKind.BYTES_KIND, new byte[0]);
        bytesBody = new BytesBodyFacade(payload.theBody);
        setBodyState(new MessageStateBodyWriteOnly(this));
    }

    public long getBodyLength() throws JMSException {
        bytesBody.absorbTheBytesBody();
        return bytesBody.getBodyLength();
    }

    public boolean readBoolean() throws JMSException {
        getBodyState().checkReadable();
        bytesBody.absorbTheBytesBody();
        return bytesBody.readBoolean();
    }

    public byte readByte() throws JMSException {
        getBodyState().checkReadable();
        bytesBody.absorbTheBytesBody();
        return bytesBody.readByte();
    }

    public int readUnsignedByte() throws JMSException {
        getBodyState().checkReadable();
        bytesBody.absorbTheBytesBody();
        return bytesBody.readUnsignedByte();
    }

    public short readShort() throws JMSException {
        getBodyState().checkReadable();
        bytesBody.absorbTheBytesBody();
        return bytesBody.readShort();
    }

    public int readUnsignedShort() throws JMSException {
        getBodyState().checkReadable();
        bytesBody.absorbTheBytesBody();
        return bytesBody.readUnsignedShort();
    }

    public char readChar() throws JMSException {
        getBodyState().checkReadable();
        bytesBody.absorbTheBytesBody();
        return bytesBody.readChar();
    }

    public int readInt() throws JMSException {
        getBodyState().checkReadable();
        bytesBody.absorbTheBytesBody();
        return bytesBody.readInt();
    }

    public long readLong() throws JMSException {
        getBodyState().checkReadable();
        bytesBody.absorbTheBytesBody();
        return bytesBody.readLong();
    }

    public float readFloat() throws JMSException {
        getBodyState().checkReadable();
        bytesBody.absorbTheBytesBody();
        return bytesBody.readFloat();
    }

    public double readDouble() throws JMSException {
        getBodyState().checkReadable();
        bytesBody.absorbTheBytesBody();
        return bytesBody.readDouble();
    }

    public String readUTF() throws JMSException {
        getBodyState().checkReadable();
        bytesBody.absorbTheBytesBody();
        return bytesBody.readUTF();
    }

    public int readBytes(byte[] bytes) throws JMSException {
        getBodyState().checkReadable();
        bytesBody.absorbTheBytesBody();
        return bytesBody.readBytes(bytes);
    }

    public int readBytes(byte[] bytes, int i) throws JMSException {
        getBodyState().checkReadable();
        bytesBody.absorbTheBytesBody();
        return bytesBody.readBytes(bytes, i);
    }

    public void writeBoolean(boolean b) throws JMSException {
        getBodyState().checkWritable();
        bytesBody.absorbTheBytesBody();
        bytesBody.writeBoolean(b);
        bytesBody.updateTheBytesBody();
    }

    public void writeByte(byte b) throws JMSException {
        getBodyState().checkWritable();
        bytesBody.absorbTheBytesBody();
        bytesBody.writeByte(b);
        bytesBody.updateTheBytesBody();
    }

    public void writeShort(short i) throws JMSException {
        getBodyState().checkWritable();
        bytesBody.absorbTheBytesBody();
        bytesBody.writeShort(i);
        bytesBody.updateTheBytesBody();
    }

    public void writeChar(char c) throws JMSException {
        getBodyState().checkWritable();
        bytesBody.absorbTheBytesBody();
        bytesBody.writeChar(c);
        bytesBody.updateTheBytesBody();
    }

    public void writeInt(int i) throws JMSException {
        getBodyState().checkWritable();
        bytesBody.absorbTheBytesBody();
        bytesBody.writeInt(i);
        bytesBody.updateTheBytesBody();
    }

    public void writeLong(long l) throws JMSException {
        getBodyState().checkWritable();
        bytesBody.absorbTheBytesBody();
        bytesBody.writeLong(l);
        bytesBody.updateTheBytesBody();
    }

    public void writeFloat(float v) throws JMSException {
        getBodyState().checkWritable();
        bytesBody.absorbTheBytesBody();
        bytesBody.writeFloat(v);
        bytesBody.updateTheBytesBody();
    }

    public void writeDouble(double v) throws JMSException {
        getBodyState().checkWritable();
        bytesBody.absorbTheBytesBody();
        bytesBody.writeDouble(v);
        bytesBody.updateTheBytesBody();
    }

    public void writeUTF(String s) throws JMSException {
        getBodyState().checkWritable();
        bytesBody.absorbTheBytesBody();
        bytesBody.writeUTF(s);
        bytesBody.updateTheBytesBody();
    }

    public void writeBytes(byte[] bytes) throws JMSException {
        getBodyState().checkWritable();
        bytesBody.absorbTheBytesBody();
        bytesBody.writeBytes(bytes);
        bytesBody.updateTheBytesBody();
    }

    public void writeBytes(byte[] bytes, int i, int i1) throws JMSException {
        getBodyState().checkWritable();
        bytesBody.absorbTheBytesBody();
        bytesBody.writeBytes(bytes, i, i1);
        bytesBody.updateTheBytesBody();
    }

    public void writeObject(Object o) throws JMSException {
        getBodyState().checkWritable();
        bytesBody.absorbTheBytesBody();
        bytesBody.writeObject(o);
        bytesBody.updateTheBytesBody();
    }

    public void reset() {
        getBodyState().makeReadable();
        bytesBody.reset();
    }

    protected void doClearBody() {
        initBody();
    }
}
