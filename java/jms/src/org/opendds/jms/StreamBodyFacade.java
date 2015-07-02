/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import javax.jms.MessageEOFException;
import javax.jms.MessageFormatException;

import OpenDDS.JMS.MessageBody;
import OpenDDS.JMS.StreamItem;

import org.opendds.jms.common.StreamItemConversion;
import org.opendds.jms.common.lang.ByteArrays;

/**
 * @author  Weiqi Gao
 */
public class StreamBodyFacade {
    private final MessageBody body;
    private final List<StreamItem> items;
    private int mark;
    private boolean byteArrayReadingInProgress;
    private int byteArrayMark;

    public StreamBodyFacade(MessageBody body) {
        this.body = body;
        this.items = new ArrayList<StreamItem>();
        this.body.theStreamBody(new StreamItem[0]);
        this.mark = 0;
    }

    public void absorbTheStreamBody() {
        if (mark == 0) {
            items.clear();
            items.addAll(Arrays.asList(body.theStreamBody()));
        }
    }

    public void updateTheStreamBody() {
        final int size = items.size();
        StreamItem[] theNewStreamBody = new StreamItem[size];
        int i = 0;
        for (StreamItem item : items) {
            theNewStreamBody[i] = item;
            i++;
        }
        body.theStreamBody(theNewStreamBody);
    }

    public boolean readBoolean() throws MessageFormatException, MessageEOFException {
        checkEndOfStreamCondition();
        checkByteArrayReadingInProgress();
        final StreamItem streamItem = items.get(mark);
        final boolean b = StreamItemConversion.convertToBoolean(streamItem);
        mark++;
        return b;
    }

    public byte readByte() throws MessageFormatException, MessageEOFException {
        checkEndOfStreamCondition();
        checkByteArrayReadingInProgress();
        final StreamItem streamItem = items.get(mark);
        final byte b = StreamItemConversion.convertToByte(streamItem);
        mark++;
        return b;
    }

    public short readShort() throws MessageFormatException, MessageEOFException {
        checkEndOfStreamCondition();
        checkByteArrayReadingInProgress();
        final StreamItem streamItem = items.get(mark);
        final short i = StreamItemConversion.convertToShort(streamItem);
        mark++;
        return i;
    }

    public char readChar() throws MessageFormatException, MessageEOFException {
        checkEndOfStreamCondition();
        checkByteArrayReadingInProgress();
        final StreamItem streamItem = items.get(mark);
        final char c = StreamItemConversion.convertToChar(streamItem);
        mark++;
        return c;
    }

    public int readInt() throws MessageFormatException, MessageEOFException {
        checkEndOfStreamCondition();
        checkByteArrayReadingInProgress();
        final StreamItem streamItem = items.get(mark);
        final int i = StreamItemConversion.convertToInt(streamItem);
        mark++;
        return i;

    }

    public long readLong() throws MessageFormatException, MessageEOFException {
        checkEndOfStreamCondition();
        checkByteArrayReadingInProgress();
        final StreamItem streamItem = items.get(mark);
        final long l = StreamItemConversion.convertToLong(streamItem);
        mark++;
        return l;
    }

    public float readFloat() throws MessageFormatException, MessageEOFException {
        checkEndOfStreamCondition();
        checkByteArrayReadingInProgress();
        final StreamItem streamItem = items.get(mark);
        final float v = StreamItemConversion.convertToFloat(streamItem);
        mark++;
        return v;
    }

    public double readDouble() throws MessageFormatException, MessageEOFException {
        checkEndOfStreamCondition();
        checkByteArrayReadingInProgress();
        final StreamItem streamItem = items.get(mark);
        final double v = StreamItemConversion.convertToDouble(streamItem);
        mark++;
        return v;
    }

    public String readString() throws MessageFormatException, MessageEOFException {
        checkEndOfStreamCondition();
        checkByteArrayReadingInProgress();
        final StreamItem streamItem = items.get(mark);
        final String s = StreamItemConversion.convertToString(streamItem);
        mark++;
        return s;
    }

    public int readBytes(byte[] bytes) throws MessageFormatException, MessageEOFException {
        checkEndOfStreamCondition();
        if (bytes == null) return -1;
        final StreamItem streamItem = items.get(mark);
        final byte[] bytes1 = StreamItemConversion.convertToByteArray(streamItem);
        if (byteArrayReadingInProgress && byteArrayMark == bytes1.length) {
            endByteArrayReading();
            return -1;
        }
        byteArrayReadingInProgress = true;
        final int filled = ByteArrays.fillByteArray(bytes1, byteArrayMark, bytes);
        byteArrayMark += filled;
        if (byteArrayMark == bytes1.length && filled < bytes.length) {
            endByteArrayReading();
        }
        return filled;
    }

    public Object readObject() throws MessageFormatException, MessageEOFException {
        checkEndOfStreamCondition();
        checkByteArrayReadingInProgress();
        final StreamItem streamItem = items.get(mark);
        final Object o = StreamItemConversion.convertToObject(streamItem);
        mark++;
        return o;
    }

    private void checkEndOfStreamCondition() throws MessageEOFException {
        if (mark >= items.size()) {
            throw new MessageEOFException("End of stream is reached.");
        }
    }

    private void endByteArrayReading() {
        byteArrayReadingInProgress = false;
        byteArrayMark = 0;
        mark++;
    }

    public void checkByteArrayReadingInProgress() throws MessageFormatException {
        if (byteArrayReadingInProgress) {
            throw new MessageFormatException("An byte array stream item has not be completely read yet.");
        }
    }

    public void writeBoolean(boolean b) {
        StreamItem item = new StreamItem();
        item.booleanValue(b);
        items.add(item);
    }

    public void writeByte(byte b) {
        StreamItem item = new StreamItem();
        item.byteValue(b);
        items.add(item);
    }

    public void writeShort(short i) {
        StreamItem item = new StreamItem();
        item.shortValue(i);
        items.add(item);
    }

    public void writeChar(char c) {
        StreamItem item = new StreamItem();
        item.charValue(c);
        items.add(item);
    }

    public void writeInt(int i) {
        StreamItem item = new StreamItem();
        item.intValue(i);
        items.add(item);
    }

    public void writeLong(long l) {
        StreamItem item = new StreamItem();
        item.longValue(l);
        items.add(item);
    }

    public void writeFloat(float v) {
        StreamItem item = new StreamItem();
        item.floatValue(v);
        items.add(item);
    }

    public void writeDouble(double v) {
        StreamItem item = new StreamItem();
        item.doubleValue(v);
        items.add(item);
    }

    public void writeString(String s) {
        StreamItem item = new StreamItem();
        item.stringValue(s);
        items.add(item);
    }

    public void writeBytes(byte[] bytes) {
        StreamItem item = new StreamItem();
        byte[] subBytes = ByteArrays.extractSubBytes(bytes, 0, bytes.length);
        item.byteArrayValue(subBytes);
        items.add(item);
    }

    public void writeBytes(byte[] bytes, int i, int i1) {
        StreamItem item = new StreamItem();
        byte[] subBytes = ByteArrays.extractSubBytes(bytes, i, i1);
        item.byteArrayValue(subBytes);
        items.add(item);
    }

    public void writeObject(Object o) throws MessageFormatException {
        StreamItem item = new StreamItem();
        if (o instanceof Boolean) {
            item.booleanValue((Boolean) o);
        } else if (o instanceof Byte) {
            item.byteValue((Byte) o);
        } else if (o instanceof Short) {
            item.shortValue((Short) o);
        } else if (o instanceof Character) {
            item.charValue((Character) o);
        } else if (o instanceof Integer) {
            item.intValue((Integer) o);
        } else if (o instanceof Long) {
            item.longValue((Long) o);
        } else if (o instanceof Float) {
            item.floatValue((Float) o);
        } else if (o instanceof Double) {
            item.doubleValue((Double) o);
        } else if (o instanceof String) {
            item.stringValue((String) o);
        } else if (o instanceof byte[]) {
            item.byteArrayValue((byte[]) o);
        } else {
            throw new MessageFormatException("Invalid value type passed in for writeObject()");
        }
        items.add(item);
    }

    public void reset() {
        this.mark = 0;
    }
}
