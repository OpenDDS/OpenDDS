/*
 * $Id$
 */

package org.opendds.jms.management;

import java.io.Serializable;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Map;
import javax.management.Attribute;
import javax.management.AttributeList;
import javax.management.AttributeNotFoundException;
import javax.management.InvalidAttributeValueException;
import javax.management.MBeanAttributeInfo;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class Attributes implements Serializable {
    private Map<String, MBeanAttributeInfo> attributeInfo =
        new LinkedHashMap<String, MBeanAttributeInfo>();

    private Map<String, Class> types =
        new HashMap<String, Class>();

    private Map<String, Object> values =
        new HashMap<String, Object>();

    public boolean contains(String attribute) {
        return attributeInfo.containsKey(attribute);
    }

    public void register(String attribute, Class type) {
        register(attribute, type, null);
    }

    public void register(String attribute, Class type, String description) {
        register(attribute, type, description, true, true);
    }

    public void registerReadOnly(String attribute, Class type) {
        registerReadOnly(attribute, type, null);
    }

    public void registerReadOnly(String attribute, Class type, String description) {
        register(attribute, type, description, true, false);
    }

    public void registerWriteOnly(String attribute, Class type) {
        registerWriteOnly(attribute, type, null);
    }

    public void registerWriteOnly(String attribute, Class type, String description) {
        register(attribute, type, description, true, false);
    }

    protected void register(String attribute,
                            Class type,
                            String description,
                            boolean isReadable,
                            boolean isWritable) {

        if (contains(attribute)) {
            throw new IllegalArgumentException("Attribute already registered: " + attribute);
        }

        types.put(attribute, type);

        attributeInfo.put(attribute, new MBeanAttributeInfo(attribute,
            type.getName(), description, isReadable, isWritable, false));
    }

    public void unregister(String attribute) {
        attributeInfo.remove(attribute);
    }

    public MBeanAttributeInfo[] getAttributeInfo() {
        return attributeInfo.values().toArray(new MBeanAttributeInfo[attributeInfo.size()]);
    }

    public Class typeOf(String attribute) {
        if (!contains(attribute)) {
            throw new IllegalArgumentException("Unknown attribute type: " + attribute);
        }
        return types.get(attribute);
    }

    //

    public Object getAttribute(String attribute) throws AttributeNotFoundException {
        if (!contains(attribute)) {
            throw new AttributeNotFoundException();
        }
        return values.get(attribute);
    }

    public void setAttribute(Attribute attribute) throws AttributeNotFoundException, InvalidAttributeValueException {
        String name = attribute.getName();
        Object value = attribute.getValue();

        MBeanAttributeInfo info = attributeInfo.get(name);
        if (info == null) {
            throw new AttributeNotFoundException();
        }

        if (!typeOf(name).isAssignableFrom(value.getClass())) {
            throw new InvalidAttributeValueException();
        }

        values.put(name, value);
    }

    public AttributeList getAttributes(String[] attributes) {
        AttributeList values = new AttributeList();

        for (String attribute : attributes) {
            try {
                values.add(new Attribute(attribute, getAttribute(attribute)));

            } catch (Exception e) {}
        }

        return values;
    }

    public AttributeList setAttributes(AttributeList attributes) {
        AttributeList values = new AttributeList();

        Iterator itr = attributes.iterator();
        while (itr.hasNext()) {
            Attribute attribute = (Attribute) itr.next();
            try {
                setAttribute(attribute);
                values.add(attribute);

            } catch (Exception e) {}
        }

        return values;
    }
}
