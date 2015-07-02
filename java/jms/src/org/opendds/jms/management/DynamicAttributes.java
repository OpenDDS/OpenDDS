/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.management;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Properties;

import javax.management.Attribute;
import javax.management.AttributeNotFoundException;
import javax.management.InvalidAttributeValueException;
import javax.management.MBeanAttributeInfo;

import org.opendds.jms.common.util.PropertiesHelper;

/**
 * @author  Steven Stallion
 */
public class DynamicAttributes implements Serializable {
    private Map<String, DynamicAttributeModel> attributes =
        new LinkedHashMap<String, DynamicAttributeModel>();

    private Map<String, String> mapped =
        new HashMap<String, String>();

    private Map<String, Object> values =
        new HashMap<String, Object>();

    public void register(String name,
                         Class type,
                         String description,
                         boolean isReadable,
                         boolean isWritable) {

        if (isRegistered(name)) {
            throw new IllegalArgumentException("Attribute already registered: " + name);
        }

        attributes.put(name, new DynamicAttributeModel(name,
            type, description, isReadable, isWritable));
    }

    public void map(String attribute, String property) {
        mapped.put(attribute, property);
    }

    public void unregister(String attribute) {
        attributes.remove(attribute);
        mapped.remove(attribute);
        values.remove(attribute);
    }

    public boolean isRegistered(String attribute) {
        return attributes.containsKey(attribute);
    }

    public boolean hasAttribute(String attribute) {
        return values.get(attribute) != null;
    }

    public Object getAttribute(String attribute) throws AttributeNotFoundException {
        if (!isRegistered(attribute)) {
            throw new AttributeNotFoundException(attribute);
        }
        return values.get(attribute);
    }

    public void setAttribute(Attribute attribute) throws AttributeNotFoundException, InvalidAttributeValueException {
        assert attribute != null;

        String name = attribute.getName();
        Object value = attribute.getValue();

        DynamicAttributeModel model = attributes.get(name);
        if (model == null) {
            throw new AttributeNotFoundException(name);
        }

        if (value != null && !model.isInstance(value)) {
            throw new InvalidAttributeValueException(value.toString());
        }

        values.put(name, value);
    }

    public Collection<MBeanAttributeInfo> getAttributeInfo() {
        Collection<MBeanAttributeInfo> values = new ArrayList<MBeanAttributeInfo>();

        for (DynamicAttributeModel model : attributes.values()) {
            values.add(model.toAttributeInfo());
        }

        return values;
    }

    public Properties toProperties() {
        Properties properties = new Properties();

        for (Map.Entry<String, Object> entry : values.entrySet()) {
            Object value = entry.getValue();
            if (value != null) {
                properties.setProperty(entry.getKey(), value.toString());
            }
        }
        return PropertiesHelper.remap(properties, mapped);
    }

    //

    public static class DynamicAttributeModel {
        private String name;
        private String description;

        private Class type;

        private boolean isReadable;
        private boolean isWritable;

        public DynamicAttributeModel(String name,
                              Class type,
                              String description,
                              boolean isReadable,
                              boolean isWritable) {
            assert name != null;
            assert type != null;

            this.name = name;
            this.description = description;
            this.type = type;
            this.isReadable = isReadable;
            this.isWritable = isWritable;
        }

        public String getName() {
            return name;
        }

        public String getDescription() {
            return description;
        }

        public Class getType() {
            return type;
        }

        public boolean isInstance(Object value) {
            return type.isInstance(value);
        }

        public MBeanAttributeInfo toAttributeInfo() {
            return new MBeanAttributeInfo(name,
                type.getName(), description, isReadable, isWritable, false);
        }
    }
}
