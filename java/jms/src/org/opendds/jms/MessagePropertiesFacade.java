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

import OpenDDS.JMS.MessagePayload;
import OpenDDS.JMS.MessageProperty;
import OpenDDS.JMS.PropertyValue;

import org.opendds.jms.common.Identifiers;
import org.opendds.jms.common.PropertyValueConversion;
import org.opendds.jms.common.lang.Objects;

/**
 * A facade over payload.theProperties, which, being a Java array
 * is hard to manage
 *
 * @author  Weiqi Gao
 */
public class MessagePropertiesFacade {
    private final MessagePayload payload;
    private final Map<String, PropertyValue> properties;

    public MessagePropertiesFacade(MessagePayload payload) {
        Objects.ensureNotNull(payload);
        this.payload = payload;
        this.properties = new HashMap<String, PropertyValue>();
    }

    public void absorbTheProperties() {
        properties.clear();
        for (MessageProperty property : payload.theProperties) {
            properties.put(property.name, property.value);
        }
    }

    public void updateTheProperties() {
        final Set<Map.Entry<String, PropertyValue>> entries = properties.entrySet();
        int size = entries.size();
        MessageProperty[] theNewProperties = new MessageProperty[size];
        int i = 0;
        for (Map.Entry<String, PropertyValue> entry : entries) {
            theNewProperties[i] = new MessageProperty(entry.getKey(), entry.getValue());
            i++;
        }
        payload.theProperties = theNewProperties;
    }

    public void clearProperties() {
        properties.clear();
    }

    public boolean propertyExists(String s) {
        ensureValidPropertyName(s);
        return properties.containsKey(s);
    }

    // JMS 1.1, 3.8.1.1, identifiers
    private void ensureValidPropertyName(String s) {
        if (!Identifiers.isValidIdentifier(s)) {
            throw new IllegalArgumentException("The string \"" + s + "\" is not a valid JMS 1.1 property identifier");
        }
    }

    public boolean getBooleanProperty(String s) throws MessageFormatException {
        ensureValidPropertyName(s);
        final PropertyValue value = properties.get(s);
        return PropertyValueConversion.convertToBoolean(value);
    }

    public byte getByteProperty(String s) throws MessageFormatException {
        ensureValidPropertyName(s);
        final PropertyValue value = properties.get(s);
        return PropertyValueConversion.convertToByte(value);
    }

    public short getShortProperty(String s) throws MessageFormatException {
        ensureValidPropertyName(s);
        final PropertyValue value = properties.get(s);
        return PropertyValueConversion.convertToShort(value);
    }

    public int getIntProperty(String s) throws MessageFormatException {
        ensureValidPropertyName(s);
        final PropertyValue value = properties.get(s);
        return PropertyValueConversion.convertToInt(value);
    }

    public long getLongProperty(String s) throws MessageFormatException {
        ensureValidPropertyName(s);
        final PropertyValue value = properties.get(s);
        return PropertyValueConversion.convertToLong(value);
    }

    public float getFloatProperty(String s) throws MessageFormatException {
        ensureValidPropertyName(s);
        final PropertyValue value = properties.get(s);
        return PropertyValueConversion.convertToFloat(value);
    }

    public double getDoubleProperty(String s) throws MessageFormatException {
        ensureValidPropertyName(s);
        final PropertyValue value = properties.get(s);
        return PropertyValueConversion.convertToDouble(value);
    }

    public String getStringProperty(String s) throws MessageFormatException {
        ensureValidPropertyName(s);
        final PropertyValue value = properties.get(s);
        return PropertyValueConversion.convertToString(value);
    }

    public Object getObjectProperty(String s) throws MessageFormatException {
        ensureValidPropertyName(s);
        final PropertyValue value = properties.get(s);
        return PropertyValueConversion.convertToObject(value);
    }

    public Enumeration getPropertyNames() {
        final Iterator<String> iter = properties.keySet().iterator();
        return new Enumeration() {
            public boolean hasMoreElements() {
                return iter.hasNext();
            }

            public Object nextElement() {
                return iter.next();
            }
        };
    }

    public void setBooleanProperty(String s, boolean b) {
        PropertyValue value = getOrCreatePropertyValue(s);
        value.booleanValue(b);
        properties.put(s, value);
    }

    private PropertyValue getOrCreatePropertyValue(String s) {
        ensureValidPropertyName(s);
        PropertyValue value = properties.get(s);
        if (value == null) value = new PropertyValue();
        return value;
    }

    public void setByteProperty(String s, byte b) {
        PropertyValue value = getOrCreatePropertyValue(s);
        value.byteValue(b);
        properties.put(s, value);
    }

    public void setShortProperty(String s, short i) {
        PropertyValue value = getOrCreatePropertyValue(s);
        value.shortValue(i);
        properties.put(s, value);
    }

    public void setIntProperty(String s, int i) {
        PropertyValue value = getOrCreatePropertyValue(s);
        value.intValue(i);
        properties.put(s, value);
    }

    public void setLongProperty(String s, long l) {
        PropertyValue value = getOrCreatePropertyValue(s);
        value.longValue(l);
        properties.put(s, value);
    }

    public void setFloatProperty(String s, float v) {
        PropertyValue value = getOrCreatePropertyValue(s);
        value.floatValue(v);
        properties.put(s, value);
    }

    public void setDoubleProperty(String s, double v) {
        PropertyValue value = getOrCreatePropertyValue(s);
        value.doubleValue(v);
        properties.put(s, value);
    }

    public void setStringProperty(String s, String s1) {
        PropertyValue value = getOrCreatePropertyValue(s);
        value.stringValue(s1);
        properties.put(s, value);
    }

    public void setObjectProperty(String s, Object o) throws MessageFormatException {
        PropertyValue value = getOrCreatePropertyValue(s);
        if (o instanceof Boolean) {
            value.booleanValue((Boolean) o);
        } else if (o instanceof Byte) {
            value.byteValue((Byte) o);
        } else if (o instanceof Short) {
            value.shortValue((Short) o);
        } else if (o instanceof Integer) {
            value.intValue((Integer) o);
        } else if (o instanceof Long) {
            value.longValue((Long) o);
        } else if (o instanceof Float) {
            value.floatValue((Float) o);
        } else if (o instanceof Double) {
            value.doubleValue((Double) o);
        } else if (o instanceof String) {
            value.stringValue((String) o);
        } else {
            throw new MessageFormatException("Invalid value passed in for setObjectProperty()");
        }
        properties.put(s, value);
    }
}
