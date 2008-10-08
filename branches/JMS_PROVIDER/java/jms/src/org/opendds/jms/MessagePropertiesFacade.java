package org.opendds.jms;

import OpenDDS.JMS.MessagePayload;
import OpenDDS.JMS.MessageProperty;
import OpenDDS.JMS.PropertyValue;
import java.util.Enumeration;
import java.util.Map;
import java.util.Set;
import java.util.HashMap;
import java.util.Iterator;
import org.opendds.jms.util.Objects;
import org.omg.CORBA.SystemException;

/**
 * A facade over payload.theProperties, which, being a Java array
 * is hard to manage
 */
public class MessagePropertiesFacade {
    private final MessagePayload payload;
    private Map<String, PropertyValue> properties;

    public MessagePropertiesFacade(MessagePayload payload) {
        Objects.ensureNotNull(payload);
        this.payload = payload;
        this.properties = new HashMap<String, PropertyValue>();
    }

    public void absorbTheProperties() { // TODO add locking
        properties.clear();
        for (MessageProperty property : payload.theProperties) {
            properties.put(property.name, property.value);
        }
    }

    public void updateTheProperties() { // TODO add locking
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
        return properties.containsKey(s);
    }

    public boolean getBooleanProperty(String s) {
        return properties.get(s).booleanValue();
    }

    public byte getByteProperty(String s) {
        return properties.get(s).byteValue();
    }

    public short getShortProperty(String s) {
        return properties.get(s).shortValue();
    }

    public int getIntProperty(String s) {
        return properties.get(s).intValue();
    }

    public long getLongProperty(String s) {
        return properties.get(s).longValue();
    }

    public float getFloatProperty(String s) {
        return properties.get(s).floatValue();
    }

    public double getDoubleProperty(String s) {
        return properties.get(s).doubleValue();
    }

    public String getStringProperty(String s) {
        return properties.get(s).stringValue();
    }

    public Object getObjectProperty(String s) {
        final PropertyValue value = properties.get(s);
        return extractObjectFromPropertyValue(value);
    }

    private Object extractObjectFromPropertyValue(final PropertyValue value) {
        final PropertyValueObjectExtractor objectExtractor = new PropertyValueObjectExtractor(value);
        return objectExtractor.extractObject();
    }

    public Enumeration getPropertyNames() {
        final Iterator<String> iter = properties.keySet().iterator();
        return new Enumeration () {
            public boolean hasMoreElements() {
                return iter.hasNext();
            }
            public Object nextElement() {
                return iter.next();
            }
        };
    }

    public void setBooleanProperty(String s, boolean b) {
        PropertyValue value = properties.get(s);
        if (value == null) value = new PropertyValue();
        value.booleanValue(b);
        properties.put(s, value);
    }

    public void setByteProperty(String s, byte b) {
        PropertyValue value = properties.get(s);
        if (value == null) value = new PropertyValue();
        value.byteValue(b);
        properties.put(s, value);
    }

    public void setShortProperty(String s, short i) {
        PropertyValue value = properties.get(s);
        if (value == null) value = new PropertyValue();
        value.shortValue(i);
        properties.put(s, value);
    }

    public void setIntProperty(String s, int i) {
        PropertyValue value = properties.get(s);
        if (value == null) value = new PropertyValue();
        value.intValue(i);
        properties.put(s, value);
    }

    public void setLongProperty(String s, long l) {
        PropertyValue value = properties.get(s);
        if (value == null) value = new PropertyValue();
        value.longValue(l);
        properties.put(s, value);
    }

    public void setFloatProperty(String s, float v) {
        PropertyValue value = properties.get(s);
        if (value == null) value = new PropertyValue();
        value.floatValue(v);
        properties.put(s, value);
    }

    public void setDoubleProperty(String s, double v) {
        PropertyValue value = properties.get(s);
        if (value == null) value = new PropertyValue();
        value.doubleValue(v);
        properties.put(s, value);
    }

    public void setStringProperty(String s, String s1) {
        PropertyValue value = properties.get(s);
        if (value == null) value = new PropertyValue();
        value.stringValue(s1);
        properties.put(s, value);
    }

    public void setObjectProperty(String s, Object o) {
        PropertyValue value = properties.get(s);
        if (value == null) value = new PropertyValue();
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
        }
        properties.put(s, value);
    }

    private static class PropertyValueObjectExtractor {
        private final PropertyValue value;

        public PropertyValueObjectExtractor(PropertyValue value) {
            this.value = value;
        }

        public Object extractObject() {
            return extractBoolean();
        }

        private Object extractBoolean() {
            try {
                boolean b = value.booleanValue();
                return Boolean.valueOf(b);
            } catch (SystemException e) {
                return extractByte();
            }
        }

        private Object extractByte() {
            try {
                byte b = value.byteValue();
                return Byte.valueOf(b);
            } catch (SystemException e) {
                return extractShort();
            }
        }

        private Object extractShort() {
            try {
                short i = value.shortValue();
                return Short.valueOf(i);
            } catch (SystemException e) {
                return extractInteger();
            }
        }

        private Object extractInteger() {
            try {
                int i = value.intValue();
                return Integer.valueOf(i);
            } catch (SystemException e) {
                return extractLong();
            }
        }

        private Object extractLong() {
            try {
                long l = value.longValue();
                return Long.valueOf(l);
            } catch (SystemException e) {
                return extractFloat();
            }
        }

        private Object extractFloat() {
            try {
                float v = value.floatValue();
                return Float.valueOf(v);
            } catch (SystemException e) {
                return extractDouble();
            }
        }

        private Object extractDouble() {
            try {
                double v = value.doubleValue();
                return Double.valueOf(v);
            } catch (SystemException e) {
                return extractString();
            }
        }

        private Object extractString() {
            try {
                return value.stringValue();
            } catch (SystemException e) {
                return null;
            }
        }

    }
}
