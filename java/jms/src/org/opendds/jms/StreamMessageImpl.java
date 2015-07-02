/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import javax.jms.JMSException;
import javax.jms.StreamMessage;

import OpenDDS.JMS.MessagePayload;
import OpenDDS.JMS.StreamItem;

/**
 * @author  Weiqi Gao
 */
public class StreamMessageImpl extends AbstractMessageImpl implements StreamMessage {
    protected StreamBodyFacade streamBody;

    public StreamMessageImpl(SessionImpl sessionImpl) {
        super(sessionImpl);
        initBody();
    }

    public StreamMessageImpl(MessagePayload messagePayload, int handle, SessionImpl sessionImpl) {
        super(messagePayload, handle, sessionImpl);
        setBodyState(new MessageStateBodyReadOnly(this));
        streamBody.reset();
    }

    private void initBody() {
        payload.theBody.theStreamBody(new StreamItem[0]);
        streamBody = new StreamBodyFacade(payload.theBody);
        setBodyState(new MessageStateBodyWriteOnly(this));
    }

    public boolean readBoolean() throws JMSException {
        getBodyState().checkReadable();
        streamBody.absorbTheStreamBody();
        return streamBody.readBoolean();
    }

    public byte readByte() throws JMSException {
        getBodyState().checkReadable();
        streamBody.absorbTheStreamBody();
        return streamBody.readByte();
    }

    public short readShort() throws JMSException {
        getBodyState().checkReadable();
        streamBody.absorbTheStreamBody();
        return streamBody.readShort();
    }

    public char readChar() throws JMSException {
        getBodyState().checkReadable();
        streamBody.absorbTheStreamBody();
        return streamBody.readChar();
    }

    public int readInt() throws JMSException {
        getBodyState().checkReadable();
        streamBody.absorbTheStreamBody();
        return streamBody.readInt();
    }

    public long readLong() throws JMSException {
        getBodyState().checkReadable();
        streamBody.absorbTheStreamBody();
        return streamBody.readLong();
    }

    public float readFloat() throws JMSException {
        getBodyState().checkReadable();
        streamBody.absorbTheStreamBody();
        return streamBody.readFloat();
    }

    public double readDouble() throws JMSException {
        getBodyState().checkReadable();
        streamBody.absorbTheStreamBody();
        return streamBody.readDouble();
    }

    public String readString() throws JMSException {
        getBodyState().checkReadable();
        streamBody.absorbTheStreamBody();
        return streamBody.readString();
    }

    public int readBytes(byte[] bytes) throws JMSException {
        getBodyState().checkReadable();
        streamBody.absorbTheStreamBody();
        return streamBody.readBytes(bytes);
    }

    public Object readObject() throws JMSException {
        getBodyState().checkReadable();
        streamBody.absorbTheStreamBody();
        return streamBody.readObject();
    }

    public void writeBoolean(boolean b) throws JMSException {
        getBodyState().checkWritable();
        streamBody.absorbTheStreamBody();
        streamBody.writeBoolean(b);
        streamBody.updateTheStreamBody();
    }

    public void writeByte(byte b) throws JMSException {
        getBodyState().checkWritable();
        streamBody.absorbTheStreamBody();
        streamBody.writeByte(b);
        streamBody.updateTheStreamBody();
    }

    public void writeShort(short i) throws JMSException {
        getBodyState().checkWritable();
        streamBody.absorbTheStreamBody();
        streamBody.writeShort(i);
        streamBody.updateTheStreamBody();
    }

    public void writeChar(char c) throws JMSException {
        getBodyState().checkWritable();
        streamBody.absorbTheStreamBody();
        streamBody.writeChar(c);
        streamBody.updateTheStreamBody();
    }

    public void writeInt(int i) throws JMSException {
        getBodyState().checkWritable();
        streamBody.absorbTheStreamBody();
        streamBody.writeInt(i);
        streamBody.updateTheStreamBody();
    }

    public void writeLong(long l) throws JMSException {
        getBodyState().checkWritable();
        streamBody.absorbTheStreamBody();
        streamBody.writeLong(l);
        streamBody.updateTheStreamBody();
    }

    public void writeFloat(float v) throws JMSException {
        getBodyState().checkWritable();
        streamBody.absorbTheStreamBody();
        streamBody.writeFloat(v);
        streamBody.updateTheStreamBody();
    }

    public void writeDouble(double v) throws JMSException {
        getBodyState().checkWritable();
        streamBody.absorbTheStreamBody();
        streamBody.writeDouble(v);
        streamBody.updateTheStreamBody();
    }

    public void writeString(String s) throws JMSException {
        getBodyState().checkWritable();
        streamBody.absorbTheStreamBody();
        streamBody.writeString(s);
        streamBody.updateTheStreamBody();
    }

    public void writeBytes(byte[] bytes) throws JMSException {
        getBodyState().checkWritable();
        streamBody.absorbTheStreamBody();
        streamBody.writeBytes(bytes);
        streamBody.updateTheStreamBody();
    }

    public void writeBytes(byte[] bytes, int i, int i1) throws JMSException {
        getBodyState().checkWritable();
        streamBody.absorbTheStreamBody();
        streamBody.writeBytes(bytes, i, i1);
        streamBody.updateTheStreamBody();
    }

    public void writeObject(Object o) throws JMSException {
        getBodyState().checkWritable();
        streamBody.absorbTheStreamBody();
        streamBody.writeObject(o);
        streamBody.updateTheStreamBody();
    }

    public void reset() {
        getBodyState().makeReadable();
        streamBody.reset();
    }

    protected void doClearBody() {
        initBody();
    }
}
