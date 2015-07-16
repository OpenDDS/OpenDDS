/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.util.Enumeration;

import javax.jms.JMSException;
import javax.jms.MapMessage;

import OpenDDS.JMS.MapItem;
import OpenDDS.JMS.MessagePayload;

/**
 * @author  Weiqi Gao
 */
public class MapMessageImpl extends AbstractMessageImpl implements MapMessage {
    protected MapBodyFacade mapBody;

    public MapMessageImpl(SessionImpl sessionImpl) {
        super(sessionImpl);
        initBody();
    }

    public MapMessageImpl(MessagePayload messagePayload, int handle, SessionImpl sessionImpl) {
        super(messagePayload, handle, sessionImpl);
        setBodyState(new MessageStateBodyNonWritable(this));
    }

    private void initBody() {
        payload.theBody.theMapBody(new MapItem[0]);
        mapBody = new MapBodyFacade(payload.theBody);
        setBodyState(new MessageStateWritable());
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
        getBodyState().checkWritable();
        mapBody.absorbTheMapBody();
        mapBody.setBoolean(s, b);
        mapBody.updateTheMapBody();
    }

    public void setByte(String s, byte b) throws JMSException {
        getBodyState().checkWritable();
        mapBody.absorbTheMapBody();
        mapBody.setByte(s, b);
        mapBody.updateTheMapBody();
    }

    public void setShort(String s, short i) throws JMSException {
        getBodyState().checkWritable();
        mapBody.absorbTheMapBody();
        mapBody.setShort(s, i);
        mapBody.updateTheMapBody();
    }

    public void setChar(String s, char c) throws JMSException {
        getBodyState().checkWritable();
        mapBody.absorbTheMapBody();
        mapBody.setChar(s, c);
        mapBody.updateTheMapBody();
    }

    public void setInt(String s, int i) throws JMSException {
        getBodyState().checkWritable();
        mapBody.absorbTheMapBody();
        mapBody.setInt(s, i);
        mapBody.updateTheMapBody();
    }

    public void setLong(String s, long l) throws JMSException {
        getBodyState().checkWritable();
        mapBody.absorbTheMapBody();
        mapBody.setLong(s, l);
        mapBody.updateTheMapBody();
    }

    public void setFloat(String s, float v) throws JMSException {
        getBodyState().checkWritable();
        mapBody.absorbTheMapBody();
        mapBody.setFloat(s, v);
        mapBody.updateTheMapBody();
    }

    public void setDouble(String s, double v) throws JMSException {
        getBodyState().checkWritable();
        mapBody.absorbTheMapBody();
        mapBody.setDouble(s, v);
        mapBody.updateTheMapBody();
    }

    public void setString(String s, String s1) throws JMSException {
        getBodyState().checkWritable();
        mapBody.absorbTheMapBody();
        mapBody.setString(s, s1);
        mapBody.updateTheMapBody();
    }

    public void setBytes(String s, byte[] bytes) throws JMSException {
        getBodyState().checkWritable();
        mapBody.absorbTheMapBody();
        mapBody.setBytes(s, bytes);
        mapBody.updateTheMapBody();
    }

    public void setBytes(String s, byte[] bytes, int i, int i1) throws JMSException {
        getBodyState().checkWritable();
        mapBody.absorbTheMapBody();
        mapBody.setBytes(s, bytes, i, i1);
        mapBody.updateTheMapBody();
    }

    public void setObject(String s, Object o) throws JMSException {
        getBodyState().checkWritable();
        mapBody.absorbTheMapBody();
        mapBody.setObject(s, o);
        mapBody.updateTheMapBody();
    }

    public boolean itemExists(String s) throws JMSException {
        mapBody.absorbTheMapBody();
        return mapBody.itemExists(s);
    }

    protected void doClearBody() {
        initBody();
    }
}
