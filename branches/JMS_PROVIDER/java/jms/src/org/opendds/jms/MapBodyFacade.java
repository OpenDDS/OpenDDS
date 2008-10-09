package org.opendds.jms;

import OpenDDS.JMS.MessageBody;
import OpenDDS.JMS.StreamItem;
import OpenDDS.JMS.MapItem;
import java.util.Map;
import java.util.HashMap;
import java.util.Set;
import java.util.Enumeration;
import java.util.Iterator;
import org.opendds.jms.util.Objects;

public class MapBodyFacade {
    private final MessageBody body;
    private final Map<String, StreamItem> items;

    public MapBodyFacade(MessageBody body) {
        Objects.ensureNotNull(body);
        this.body = body;
        this.items = new HashMap<String, StreamItem>();
        this.body.theMapBody(new MapItem[0]);
    }

    public void absorbTheMapBody() { // TODO add locking
        items.clear();
        for (MapItem item : body.theMapBody()) {
            items.put(item.name, item.value);
        }
    }

    public void updateTheMapBody() { // TODO add locking
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

    public boolean getBoolean(String s) {
        return items.get(s).booleanValue();
    }

    public byte getByte(String s) {
        return items.get(s).byteValue();
    }

    public short getShort(String s) {
        return items.get(s).shortValue();
    }

    public char getChar(String s) {
        return items.get(s).charValue();
    }

    public int getInt(String s) {
        return items.get(s).intValue();
    }

    public long getLong(String s) {
        return items.get(s).longValue();
    }

    public float getFloat(String s) {
        return items.get(s).floatValue();
    }

    public double getDouble(String s) {
        return items.get(s).doubleValue();
    }

    public String getString(String s) {
        return items.get(s).stringValue();
    }

    public byte[] getBytes(String s) {
        return null; // TODO
    }

    public Object getObject(String s) {
        return items.get(s); // TODO
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
        StreamItem item = items.get(s);
        if (item == null) item = new StreamItem();
        item.booleanValue(b);
        items.put(s, item);
    }

    public void setByte(String s, byte b) {
        StreamItem item = items.get(s);
        if (item == null) item = new StreamItem();
        item.byteValue(b);
        items.put(s, item);
    }

    public void setShort(String s, short i) {
        StreamItem item = items.get(s);
        if (item == null) item = new StreamItem();
        item.shortValue(i);
        items.put(s, item);
    }

    public void setChar(String s, char c) {
        StreamItem item = items.get(s);
        if (item == null) item = new StreamItem();
        item.charValue(c);
        items.put(s, item);
    }

    public void setInt(String s, int i) {
        StreamItem item = items.get(s);
        if (item == null) item = new StreamItem();
        item.intValue(i);
        items.put(s, item);
    }

    public void setLong(String s, long l) {
        StreamItem item = items.get(s);
        if (item == null) item = new StreamItem();
        item.longValue(l);
        items.put(s, item);
    }

    public void setFloat(String s, float v) {
        StreamItem item = items.get(s);
        if (item == null) item = new StreamItem();
        item.floatValue(v);
        items.put(s, item);
    }

    public void setDouble(String s, double v) {
        StreamItem item = items.get(s);
        if (item == null) item = new StreamItem();
        item.doubleValue(v);
        items.put(s, item);

    }

    public void setString(String s, String s1) {
        StreamItem item = items.get(s);
        if (item == null) item = new StreamItem();
        item.stringValue(s1);
        items.put(s, item);
    }

    public void setBytes(String s, byte[] bytes) {
        StreamItem item = items.get(s);
        if (item == null) item = new StreamItem();
        byte[] subBytes = extractSubBytes(bytes, 0, bytes.length);
        item.byteArrayValue(bytes);
        items.put(s, item);
    }

    public void setBytes(String s, byte[] bytes, int i, int i1) {
        StreamItem item = items.get(s);
        if (item == null) item = new StreamItem();
        byte[] subBytes = extractSubBytes(bytes, i, i1);
        item.byteArrayValue(bytes);
        items.put(s, item);
    }

    private byte[] extractSubBytes(byte[] bytes, int i, int i1) {
        byte[] subBytes = new byte[i1];
        System.arraycopy(bytes, i, subBytes, 0, i1);
        return subBytes;
    }

    public void setObject(String s, Object o) {
        StreamItem item = items.get(s);
        if (item == null) item = new StreamItem();
//        item.objectValue(l); // TODO
        items.put(s, item);
    }

    public boolean itemExists(String s) {
        return items.containsKey(s);
    }
}
