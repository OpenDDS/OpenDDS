package org.opendds.jms;

import javax.jms.Message;
import javax.jms.JMSException;
import javax.jms.Destination;
import java.util.Enumeration;
import java.io.UnsupportedEncodingException;
import OpenDDS.JMS.MessagePayload;
import OpenDDS.JMS.MessageHeaders;
import OpenDDS.JMS.MessageProperty;
import OpenDDS.JMS.MessageBody;

public abstract class AbstractMessageImpl implements Message {
    protected final MessagePayload payload;
    protected final MessageHeaders headers;
    protected final MessageProperty[] properties;
    protected final MessageBody body;

    protected AbstractMessageImpl() {
        this.payload = new MessagePayload(new MessageHeaders(), null, new MessageBody());
        headers = payload.theHeaders;
        properties = payload.theProperties;
        body = payload.theBody;
    }

    public String getJMSMessageID() throws JMSException {
        return headers.JMSMessageID;
    }

    public void setJMSMessageID(String messageID) throws JMSException {
        headers.JMSMessageID = messageID;
    }

    public long getJMSTimestamp() throws JMSException {
        return headers.JMSTimestamp;
    }

    public void setJMSTimestamp(long timestamp) throws JMSException {
        headers.JMSTimestamp = timestamp;
    }

    public byte[] getJMSCorrelationIDAsBytes() throws JMSException {
        try {
            return headers.JMSCorrelationID.getBytes("UTF-8");
        } catch (UnsupportedEncodingException e) {
            return null; // Can't happen
        }
    }

    public void setJMSCorrelationIDAsBytes(byte[] correlationID) throws JMSException {
        try {
            headers.JMSCorrelationID = new String(correlationID, "UTF-8");
        } catch (UnsupportedEncodingException e) {
            headers.JMSCorrelationID = ""; // Can't happen
        }
    }

    public void setJMSCorrelationID(String correlationID) throws JMSException {
        headers.JMSCorrelationID = correlationID;
    }

    public String getJMSCorrelationID() throws JMSException {
        return headers.JMSCorrelationID;
    }

    public Destination getJMSReplyTo() throws JMSException {
        return DestinationImpl.fromString(headers.JMSReplyTo);
    }

    public void setJMSReplyTo(Destination destination) throws JMSException {
        headers.JMSReplyTo = destination.toString();
    }

    public Destination getJMSDestination() throws JMSException {
        return DestinationImpl.fromString(headers.JMSDestination);
    }

    public void setJMSDestination(Destination destination) throws JMSException {
        headers.JMSDestination = destination.toString();
    }

    public int getJMSDeliveryMode() throws JMSException {
        return headers.JMSDeliveryMode;
    }

    public void setJMSDeliveryMode(int deliveryMode) throws JMSException {
        headers.JMSDeliveryMode = deliveryMode;
    }

    public boolean getJMSRedelivered() throws JMSException {
        return headers.JMSRedelivered;
    }

    public void setJMSRedelivered(boolean redelivered) throws JMSException {
        headers.JMSRedelivered = redelivered;
    }

    public String getJMSType() throws JMSException {
        return headers.JMSType;
    }

    public void setJMSType(String type) throws JMSException {
        headers.JMSType = type;
    }

    public long getJMSExpiration() throws JMSException {
        return headers.JMSExpiration;
    }

    public void setJMSExpiration(long expiration) throws JMSException {
        headers.JMSExpiration = expiration;
    }

    public int getJMSPriority() throws JMSException {
        return headers.JMSPriority;
    }

    public void setJMSPriority(int priority) throws JMSException {
        headers.JMSPriority = priority;
    }

    public void clearProperties() throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public boolean propertyExists(String s) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public boolean getBooleanProperty(String s) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public byte getByteProperty(String s) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public short getShortProperty(String s) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public int getIntProperty(String s) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public long getLongProperty(String s) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public float getFloatProperty(String s) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public double getDoubleProperty(String s) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public String getStringProperty(String s) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public Object getObjectProperty(String s) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public Enumeration getPropertyNames() throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public void setBooleanProperty(String s, boolean b) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public void setByteProperty(String s, byte b) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public void setShortProperty(String s, short i) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public void setIntProperty(String s, int i) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public void setLongProperty(String s, long l) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public void setFloatProperty(String s, float v) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public void setDoubleProperty(String s, double v) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public void setStringProperty(String s, String s1) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public void setObjectProperty(String s, Object o) throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public void acknowledge() throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    public void clearBody() throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }
}
