package org.opendds.jms;

import javax.jms.Message;
import javax.jms.JMSException;
import javax.jms.Destination;
import java.util.Enumeration;
import OpenDDS.JMS.MessagePayload;
import OpenDDS.JMS.MessageHeader;
import OpenDDS.JMS.MessageProperty;
import OpenDDS.JMS.MessageBody;

public abstract class AbstractMessageImpl implements Message {
    protected final MessagePayload payload;

    // Convenience variables, should be kept in sync with payload
    protected final MessageHeader headers;
    protected final MessagePropertiesFacade properties;

    private MessageState propertiesState;
    private MessageState bodyState;

    protected AbstractMessageImpl() {
        this.payload = new MessagePayload(new MessageHeader(), new MessageProperty[0], new MessageBody());
        this.headers = payload.theHeader;
        this.properties = new MessagePropertiesFacade(payload);
        this.propertiesState = new MessageStateWritable();
    }

    public void setPropertiesState(MessageState propertiesState) {
        this.propertiesState = propertiesState;
    }

    public MessageState getPropertiesState() {
        return propertiesState;
    }

    public MessageState getBodyState() {
        return bodyState;
    }

    public void setBodyState(MessageState bodyState) {
        this.bodyState = bodyState;
    }

    // Message headers
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
        throw new java.lang.UnsupportedOperationException("getJMSCorrelationIDAsBytes() is not supported.");
    }

    public void setJMSCorrelationIDAsBytes(byte[] correlationID) throws JMSException {
        throw new java.lang.UnsupportedOperationException("setJMSCorrelationIDAsBytes() is not supported.");
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

    // Message properties
    public void clearProperties() throws JMSException {
        // JMS 1.1, 3.10
        propertiesState.makeWritable();
        properties.absorbTheProperties();
        properties.clearProperties();
        properties.updateTheProperties();
    }

    public boolean propertyExists(String s) throws JMSException {
        properties.absorbTheProperties();
        return properties.propertyExists(s);
    }

    public boolean getBooleanProperty(String s) throws JMSException {
        properties.absorbTheProperties();
        return properties.getBooleanProperty(s);
    }

    public byte getByteProperty(String s) throws JMSException {
        properties.absorbTheProperties();
        return properties.getByteProperty(s);
    }

    public short getShortProperty(String s) throws JMSException {
        properties.absorbTheProperties();
        return properties.getShortProperty(s);

    }

    public int getIntProperty(String s) throws JMSException {
        properties.absorbTheProperties();
        return properties.getIntProperty(s);
    }

    public long getLongProperty(String s) throws JMSException {
        properties.absorbTheProperties();
        return properties.getLongProperty(s);
    }

    public float getFloatProperty(String s) throws JMSException {
        properties.absorbTheProperties();
        return properties.getFloatProperty(s);
    }

    public double getDoubleProperty(String s) throws JMSException {
        properties.absorbTheProperties();
        return properties.getDoubleProperty(s);
    }

    public String getStringProperty(String s) throws JMSException {
        properties.absorbTheProperties();
        return properties.getStringProperty(s);
    }

    public Object getObjectProperty(String s) throws JMSException {
        properties.absorbTheProperties();
        return properties.getObjectProperty(s);
    }

    public Enumeration getPropertyNames() throws JMSException {
        properties.absorbTheProperties();
        return properties.getPropertyNames();
    }

    public void setBooleanProperty(String s, boolean b) throws JMSException {
        propertiesState.checkWritable();
        properties.absorbTheProperties();
        properties.setBooleanProperty(s, b);
        properties.updateTheProperties();
    }

    public void setByteProperty(String s, byte b) throws JMSException {
        propertiesState.checkWritable();
        properties.absorbTheProperties();
        properties.setByteProperty(s, b);
        properties.updateTheProperties();
    }

    public void setShortProperty(String s, short i) throws JMSException {
        propertiesState.checkWritable();
        properties.absorbTheProperties();
        properties.setShortProperty(s, i);
        properties.updateTheProperties();
    }

    public void setIntProperty(String s, int i) throws JMSException {
        propertiesState.checkWritable();
        properties.absorbTheProperties();
        properties.setIntProperty(s, i);
        properties.updateTheProperties();
    }

    public void setLongProperty(String s, long l) throws JMSException {
        propertiesState.checkWritable();
        properties.absorbTheProperties();
        properties.setLongProperty(s, l);
        properties.updateTheProperties();
    }

    public void setFloatProperty(String s, float v) throws JMSException {
        propertiesState.checkWritable();
        properties.absorbTheProperties();
        properties.setFloatProperty(s, v);
        properties.updateTheProperties();
    }

    public void setDoubleProperty(String s, double v) throws JMSException {
        propertiesState.checkWritable();
        properties.absorbTheProperties();
        properties.setDoubleProperty(s, v);
        properties.updateTheProperties();
    }

    public void setStringProperty(String s, String s1) throws JMSException {
        propertiesState.checkWritable();
        properties.absorbTheProperties();
        properties.setStringProperty(s, s1);
        properties.updateTheProperties();
    }

    public void setObjectProperty(String s, Object o) throws JMSException {
        propertiesState.checkWritable();
        properties.absorbTheProperties();
        properties.setObjectProperty(s, o);
        properties.updateTheProperties();
    }

    public void acknowledge() throws JMSException {
        throw new UnsupportedOperationException("Kaboom"); // TODO
    }

    // Message body
    public void clearBody() throws JMSException {
        // JMS 1.1, 3.10
        bodyState.makeWritable();
        payload.theBody = new MessageBody();
        doClearBody();
    }

    /**
     * Subclass should implement this method by initializing payload.theBody with the
     * appropriate kind of body.
     */
    protected abstract void doClearBody();
}
