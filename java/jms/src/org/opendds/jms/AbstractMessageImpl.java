/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.util.Enumeration;

import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.Session;

import OpenDDS.JMS.MessageBody;
import OpenDDS.JMS.MessageHeader;
import OpenDDS.JMS.MessagePayload;
import OpenDDS.JMS.MessageProperty;

/**
 * @author  Weiqi Gao
 */
public abstract class AbstractMessageImpl implements Message {
    protected final MessagePayload payload;
    protected int handle;

    // Convenience variables, should be kept in sync with payload
    protected final MessageHeader headers;
    protected final MessagePropertiesFacade properties;

    private MessageState propertiesState;
    private MessageState bodyState;

    // JMS 1.1, 4.4.11, acknowledgement is a session-wide concept;
    protected SessionImpl sessionImpl;

    /**
     * Construct a Message on the Producer side.
     */
    protected AbstractMessageImpl(SessionImpl sessionImpl) {
        this.payload = new MessagePayload(new MessageHeader(), new MessageProperty[0], new MessageBody());
        this.headers = payload.theHeader;
        this.properties = new MessagePropertiesFacade(payload);
        this.propertiesState = new MessageStateWritable();
        this.sessionImpl = sessionImpl;
    }

    /**
     * Construct a Message on the Consumer side.
     *
     * @param messagePayload The MessagePayload of the Message
     * @param handle The DDS instance instance handle of the MessagePayload
     * @param sessionImpl
     */
    public AbstractMessageImpl(MessagePayload messagePayload, int handle, SessionImpl sessionImpl) {
        this.payload = messagePayload;
        this.headers = payload.theHeader;
        this.properties = new MessagePropertiesFacade(payload);
        this.propertiesState = new MessageStatePropertiesNonWritable(this);
        this.handle = handle;
        this.sessionImpl = sessionImpl;
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

    public MessagePayload getPayload() {
        return payload;
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
        return new TopicImpl(headers.JMSReplyTo);
    }

    public void setJMSReplyTo(Destination destination) throws JMSException {
        headers.JMSReplyTo = destination.toString();
    }

    public Destination getJMSDestination() throws JMSException {
        return new TopicImpl(headers.JMSDestination);
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

    public void setJMSRedelivered(boolean redelivered) {
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

    // We use (20 - JMSPriority) in the IDL so that we can use "ORDER BY theHeader.TwentyMinusJMSPriority"
    // in a QueryCondition in DDS to read the higher JMSPriority samples from the DataReader before lower JMSPriority
    // samples.
    public int getJMSPriority() throws JMSException {
        return 20 - headers.TwentyMinusJMSPriority;
    }

    public void setJMSPriority(int priority) throws JMSException {
        headers.TwentyMinusJMSPriority = 20 - priority;
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

    /**
     * Send an acknowledgement to the session from where this Message is delivered.
     * JMS 1.1, 4.4.11, this acknowledges this and all other Messages that have been
     * delivered by its session.
     *
     * @throws JMSException
     */
    public void acknowledge() throws JMSException {
        sessionImpl.checkClosed();
        if (sessionImpl.getAcknowledgeMode() != Session.CLIENT_ACKNOWLEDGE) return;
        sessionImpl.doAcknowledge();
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
