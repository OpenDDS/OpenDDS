package org.opendds.jms;

import javax.jms.MapMessage;
import javax.jms.JMSException;
import java.util.Enumeration;

public class MapMessageImpl extends AbstractMessageImpl implements MapMessage {
    protected final MapBodyFacade mapBody;

    public MapMessageImpl() {
        mapBody = new MapBodyFacade(body);
    }

    public boolean getBoolean(String s) throws JMSException {
        mapBody.absorbTheMapBody();
        return mapBody.getBoolean(s);
    }

    public byte getByte(String s) throws JMSException {
        mapBody.absorbTheMapBody();
        return mapBody.getByte(s);
    }

    public short getShort(String s) throws JMSException {
        mapBody.absorbTheMapBody();
        return mapBody.getShort(s);
    }

    public char getChar(String s) throws JMSException {
        mapBody.absorbTheMapBody();
        return mapBody.getChar(s);
    }

    public int getInt(String s) throws JMSException {
        mapBody.absorbTheMapBody();
        return mapBody.getInt(s);
    }

    public long getLong(String s) throws JMSException {
        mapBody.absorbTheMapBody();
        return mapBody.getLong(s);
    }

    public float getFloat(String s) throws JMSException {
        mapBody.absorbTheMapBody();
        return mapBody.getFloat(s);
    }

    public double getDouble(String s) throws JMSException {
        mapBody.absorbTheMapBody();
        return mapBody.getDouble(s);
    }

    public String getString(String s) throws JMSException {
        mapBody.absorbTheMapBody();
        return mapBody.getString(s);
    }

    public byte[] getBytes(String s) throws JMSException {
        mapBody.absorbTheMapBody();
        return mapBody.getBytes(s);
    }

    public Object getObject(String s) throws JMSException {
        mapBody.absorbTheMapBody();
        return mapBody.getObject(s);
    }

    public Enumeration getMapNames() throws JMSException {
        mapBody.absorbTheMapBody();
        return mapBody.getMapNames();
    }

    public void setBoolean(String s, boolean b) throws JMSException {
        mapBody.absorbTheMapBody();
        mapBody.setBoolean(s, b);
        mapBody.updateTheMapBody();
    }

    public void setByte(String s, byte b) throws JMSException {
        mapBody.absorbTheMapBody();
        mapBody.setByte(s, b);
        mapBody.updateTheMapBody();
    }

    public void setShort(String s, short i) throws JMSException {
        mapBody.absorbTheMapBody();
        mapBody.setShort(s, i);
        mapBody.updateTheMapBody();
    }

    public void setChar(String s, char c) throws JMSException {
        mapBody.absorbTheMapBody();
        mapBody.setChar(s, c);
        mapBody.updateTheMapBody();
    }

    public void setInt(String s, int i) throws JMSException {
        mapBody.absorbTheMapBody();
        mapBody.setInt(s, i);
        mapBody.updateTheMapBody();
    }

    public void setLong(String s, long l) throws JMSException {
        mapBody.absorbTheMapBody();
        mapBody.setLong(s, l);
        mapBody.updateTheMapBody();
    }

    public void setFloat(String s, float v) throws JMSException {
        mapBody.absorbTheMapBody();
        mapBody.setFloat(s, v);
        mapBody.updateTheMapBody();
    }

    public void setDouble(String s, double v) throws JMSException {
        mapBody.absorbTheMapBody();
        mapBody.setDouble(s, v);
        mapBody.updateTheMapBody();
    }

    public void setString(String s, String s1) throws JMSException {
        mapBody.absorbTheMapBody();
        mapBody.setString(s, s1);
        mapBody.updateTheMapBody();
    }

    public void setBytes(String s, byte[] bytes) throws JMSException {
        mapBody.absorbTheMapBody();
        mapBody.setBytes(s, bytes);
        mapBody.updateTheMapBody();
    }

    public void setBytes(String s, byte[] bytes, int i, int i1) throws JMSException {
        mapBody.absorbTheMapBody();
        mapBody.setBytes(s, bytes, i, i1);
        mapBody.updateTheMapBody();
    }

    public void setObject(String s, Object o) throws JMSException {
        mapBody.absorbTheMapBody();
        mapBody.setObject(s, o);
        mapBody.updateTheMapBody();
    }

    public boolean itemExists(String s) throws JMSException {
        mapBody.absorbTheMapBody();
        return mapBody.itemExists(s);
    }
}