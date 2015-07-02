/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

import javax.jms.MessageFormatException;

import OpenDDS.JMS.MapItem;
import OpenDDS.JMS.MessageBody;
import OpenDDS.JMS.StreamItem;

import org.opendds.jms.common.StreamItemConversion;
import org.opendds.jms.common.lang.Objects;

/**
 * @author  Weiqi Gao
 */
public class MapBodyFacade {
    private final MessageBody body;
    private final Map<String, StreamItem> items;

    public MapBodyFacade(MessageBody body) {
        Objects.ensureNotNull(body);
        this.body = body;
        this.items = new HashMap<String, StreamItem>();
        this.body.theMapBody(new MapItem[0]);
    }

    public void absorbTheMapBody() {
        items.clear();
        for (MapItem item : body.theMapBody()) {
            items.put(item.name, item.value);
        }
    }

    public void updateTheMapBody() {
        final Set<Map.Entry<String,StreamItem>> entries = items.entrySet();
        int size = entries.size();
        MapItem[] theNewMapBody = new MapItem[size];
        int i = 0;
        for (Map.Entry<String, StreamItem> entry : entries) {
            theNewMapBody[i] = new MapItem(entry.getKey(), entry.getValue());
            i++;
        }
        body.theMapBody(theNewMapBody);
    }

    public boolean getBoolean(String s) throws MessageFormatException {
        final StreamItem streamItem = items.get(s);
        return StreamItemConversion.convertToBoolean(streamItem);
    }

    public byte getByte(String s) throws MessageFormatException {
        final StreamItem streamItem = items.get(s);
        return StreamItemConversion.convertToByte(streamItem);
    }

    public short getShort(String s) throws MessageFormatException {
        final StreamItem streamItem = items.get(s);
        return StreamItemConversion.convertToShort(streamItem);
    }

    public char getChar(String s) throws MessageFormatException {
        final StreamItem streamItem = items.get(s);
        return StreamItemConversion.convertToChar(streamItem);
    }

    public int getInt(String s) throws MessageFormatException {
        final StreamItem streamItem = items.get(s);
        return StreamItemConversion.convertToInt(streamItem);
    }

    public long getLong(String s) throws MessageFormatException {
        final StreamItem streamItem = items.get(s);
        return StreamItemConversion.convertToLong(streamItem);
    }

    public float getFloat(String s) throws MessageFormatException {
        final StreamItem streamItem = items.get(s);
        return StreamItemConversion.convertToFloat(streamItem);
    }

    public double getDouble(String s) throws MessageFormatException {
        final StreamItem streamItem = items.get(s);
        return StreamItemConversion.convertToDouble(streamItem);
    }

    public String getString(String s) throws MessageFormatException {
        final StreamItem streamItem = items.get(s);
        return StreamItemConversion.convertToString(streamItem);
    }

    public byte[] getBytes(String s) throws MessageFormatException {
        final StreamItem streamItem = items.get(s);
        return StreamItemConversion.convertToByteArray(streamItem);
    }

    public Object getObject(String s) throws MessageFormatException {
        final StreamItem streamItem = items.get(s);
        return StreamItemConversion.convertToObject(streamItem);
    }

    public Enumeration getMapNames() {
        final Iterator<String> iterator = items.keySet().iterator();
        return new Enumeration() {
            public boolean hasMoreElements() {
                return iterator.hasNext();
            }
            public Object nextElement() {
                return iterator.next();
            }
        };
    }

    public void setBoolean(String s, boolean b) {
        StreamItem item = getOrCreateStreamItem(s);
        item.booleanValue(b);
        items.put(s, item);
    }

    private StreamItem getOrCreateStreamItem(String s) {
        StreamItem item = items.get(s);
        if (item == null) item = new StreamItem();
        return item;
    }

    public void setByte(String s, byte b) {
        StreamItem item = getOrCreateStreamItem(s);
        item.byteValue(b);
        items.put(s, item);
    }

    public void setShort(String s, short i) {
        StreamItem item = getOrCreateStreamItem(s);
        item.shortValue(i);
        items.put(s, item);
    }

    public void setChar(String s, char c) {
        StreamItem item = getOrCreateStreamItem(s);
        item.charValue(c);
        items.put(s, item);
    }

    public void setInt(String s, int i) {
        StreamItem item = getOrCreateStreamItem(s);
        item.intValue(i);
        items.put(s, item);
    }

    public void setLong(String s, long l) {
        StreamItem item = getOrCreateStreamItem(s);
        item.longValue(l);
        items.put(s, item);
    }

    public void setFloat(String s, float v) {
        StreamItem item = getOrCreateStreamItem(s);
        item.floatValue(v);
        items.put(s, item);
    }

    public void setDouble(String s, double v) {
        StreamItem item = getOrCreateStreamItem(s);
        item.doubleValue(v);
        items.put(s, item);

    }

    public void setString(String s, String s1) {
        StreamItem item = getOrCreateStreamItem(s);
        item.stringValue(s1);
        items.put(s, item);
    }

    public void setBytes(String s, byte[] bytes) {
        StreamItem item = getOrCreateStreamItem(s);
        byte[] subBytes = extractSubBytes(bytes, 0, bytes.length);
        item.byteArrayValue(bytes);
        items.put(s, item);
    }

    public void setBytes(String s, byte[] bytes, int i, int i1) {
        StreamItem item = getOrCreateStreamItem(s);
        byte[] subBytes = extractSubBytes(bytes, i, i1);
        item.byteArrayValue(subBytes);
        items.put(s, item);
    }

    private byte[] extractSubBytes(byte[] bytes, int i, int i1) {
        byte[] subBytes = new byte[i1];
        System.arraycopy(bytes, i, subBytes, 0, i1);
        return subBytes;
    }

    public void setObject(String s, Object o) throws MessageFormatException {
        StreamItem item = getOrCreateStreamItem(s);
        if (o instanceof Boolean) {
            item.booleanValue((Boolean) o);
        } else if (o instanceof Byte) {
            item.byteValue((Byte) o);
        } else if (o instanceof Short) {
            item.shortValue((Short) o);
        } else if (o instanceof Character){
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
            throw new MessageFormatException("Invalid value passed in for setObject()");
        }
        items.put(s, item);
    }

    public boolean itemExists(String s) {
        return items.containsKey(s);
    }
}
