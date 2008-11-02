/*
 * $Id$
 */

package org.opendds.jms.management;

import java.beans.PropertyDescriptor;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import javax.management.AttributeList;
import javax.management.AttributeNotFoundException;
import javax.management.DynamicMBean;
import javax.management.IntrospectionException;
import javax.management.InvalidAttributeValueException;
import javax.management.MBeanAttributeInfo;
import javax.management.MBeanConstructorInfo;
import javax.management.MBeanException;
import javax.management.MBeanInfo;
import javax.management.MBeanNotificationInfo;
import javax.management.MBeanOperationInfo;
import javax.management.MBeanRegistration;
import javax.management.MBeanServer;
import javax.management.ObjectName;
import javax.management.ReflectionException;

import org.opendds.jms.management.annotation.Attribute;
import org.opendds.jms.management.annotation.Constructor;
import org.opendds.jms.management.annotation.Description;
import org.opendds.jms.management.annotation.Operation;
import org.opendds.jms.util.Annotations;
import org.opendds.jms.util.BeanHelper;
import org.opendds.jms.util.Classes;
import org.opendds.jms.util.Strings;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public abstract class DynamicMBeanSupport implements DynamicMBean, MBeanRegistration {
    private Map<String, Class> attributeTypes =
        new HashMap<String, Class>();

    private Map<String, MBeanAttributeInfo> attributeInfo =
        new LinkedHashMap<String, MBeanAttributeInfo>();

    private Map<String, Object> attributeValues =
        new HashMap<String, Object>();

    protected MBeanServer server;
    protected ObjectName name;

    protected Boolean registrationDone;

    protected String getKeyProperty(String key) {
        return name.getKeyProperty(key);
    }

    protected String requireKeyProperty(String key) {
        String value = getKeyProperty(key);
        if (Strings.isEmpty(value)) {
            throw new IllegalArgumentException(key + " is a required key property!");
        }
        return value;
    }

    public ObjectName preRegister(MBeanServer server, ObjectName name) {
        this.server = server;
        this.name = name;
        return name;
    }

    public void postRegister(Boolean registrationDone) {
        this.registrationDone = registrationDone;
    }

    public void preDeregister() {}

    public void postDeregister() {}


    public boolean isAttributeRegistered(String attribute) {
        return attributeInfo.containsKey(attribute);
    }

    public void registerAttribute(String attribute, Class type) {
        registerAttribute(attribute, type, null);
    }

    public void registerAttribute(String attribute, Class type, String description) {
        registerAttribute(attribute, type, description, true, true);
    }

    public void registerReadOnlyAttribute(String attribute, Class type) {
        registerReadOnlyAttribute(attribute, type, null);
    }

    public void registerReadOnlyAttribute(String attribute, Class type, String description) {
        registerAttribute(attribute, type, description, true, false);
    }

    protected void registerAttribute(String attribute,
                                     Class type,
                                     String description,
                                     boolean isReadable,
                                     boolean isWritable) {

        if (isAttributeRegistered(attribute)) {
            throw new IllegalArgumentException("Attribute already registered: " + attribute);
        }

        attributeTypes.put(attribute, type);

        attributeInfo.put(attribute, new MBeanAttributeInfo(attribute,
            type.getName(), description, isReadable, isWritable, false));
    }

    public void unregisterAttribute(String attribute) {
        attributeTypes.remove(attribute);
        attributeInfo.remove(attribute);
        attributeValues.remove(attribute);
    }

    public Class getAttributeType(String attribute) {
        if (!isAttributeRegistered(attribute)) {
            throw new IllegalArgumentException("Unknown attribute type: " + attribute);
        }
        return attributeTypes.get(attribute);
    }

    public Object getAttribute(String attribute) throws AttributeNotFoundException {
        if (!isAttributeRegistered(attribute)) {
            throw new AttributeNotFoundException();
        }
        return attributeValues.get(attribute);
    }

    public void setAttribute(javax.management.Attribute attribute) throws AttributeNotFoundException, InvalidAttributeValueException {
        String name = attribute.getName();
        Object value = attribute.getValue();

        MBeanAttributeInfo info = attributeInfo.get(name);
        if (info == null) {
            throw new AttributeNotFoundException();
        }

        if (!getAttributeType(name).isAssignableFrom(value.getClass())) {
            throw new InvalidAttributeValueException();
        }

        attributeValues.put(name, value);
    }

    public AttributeList getAttributes(String[] attributes) {
        AttributeList values = new AttributeList();

        for (String attribute : attributes) {
            try {
                values.add(new javax.management.Attribute(attribute, getAttribute(attribute)));

            } catch (Exception e) {}
        }

        return values;
    }

    public AttributeList setAttributes(AttributeList attributes) {
        AttributeList values = new AttributeList();

        Iterator itr = attributes.iterator();
        while (itr.hasNext()) {
            javax.management.Attribute attribute = (javax.management.Attribute) itr.next();
            try {
                setAttribute(attribute);
                values.add(attribute);

            } catch (Exception e) {}
        }

        return values;
    }

    public MBeanInfo getMBeanInfo() {
        return new MBeanInfo(getClass().getName(),
                             getDescription(),
                             getAttributeInfo(),
                             getConstructorInfo(),
                             getOperationInfo(),
                             getNotificationInfo());
    }

    protected String getDescription() {
        String description = null;

        Description d = Annotations.getAnnotation(this, Description.class);
        if (d != null) {
            description = d.value();
        }

        return description;
    }

    protected MBeanAttributeInfo[] getAttributeInfo() {
        List<MBeanAttributeInfo> info = new ArrayList<MBeanAttributeInfo>();

        // Add dynamic attributes
        info.addAll(attributeInfo.values());

        // Add annotated attributes
        BeanHelper helper = new BeanHelper(this);
        for (PropertyDescriptor property : helper.findAnnotatedProperties(Attribute.class)) {
            String description = null;

            Description d = Annotations.getAnnotation(property.getReadMethod(), Description.class);
            if (d != null) {
                description = d.value();
            }

            try {
                info.add(new MBeanAttributeInfo(property.getName(), description,
                    property.getReadMethod(), property.getWriteMethod()));

            } catch (IntrospectionException e) {
                throw new IllegalArgumentException(e);
            }
        }

        return info.toArray(new MBeanAttributeInfo[info.size()]);
    }

    protected MBeanConstructorInfo[] getConstructorInfo() {
        List<MBeanConstructorInfo> info = new ArrayList<MBeanConstructorInfo>();

        for (java.lang.reflect.Constructor constructor :
                Annotations.findAnnotatedConstructors(this, Constructor.class)) {

            String description = null;

            Description d = Annotations.getAnnotation(constructor, Description.class);
            if (d != null) {
                description = d.value();
            }

            info.add(new MBeanConstructorInfo(description, constructor));
        }

        return info.toArray(new MBeanConstructorInfo[info.size()]);
    }

    protected MBeanOperationInfo[] getOperationInfo() {
        List<MBeanOperationInfo> info = new ArrayList<MBeanOperationInfo>();

        for (Method method : Annotations.findAnnotatedMethods(this, Operation.class)) {
            String description = null;

            Description d = method.getAnnotation(Description.class);
            if (d != null) {
                description = d.value();
            }

            info.add(new MBeanOperationInfo(description, method));
        }

        return info.toArray(new MBeanOperationInfo[info.size()]);
    }

    protected MBeanNotificationInfo[] getNotificationInfo() {
        return null;
    }

    public Object invoke(String actionName,
                         Object params[],
                         String signature[]) throws MBeanException, ReflectionException {
        try {
            Method method = Classes.findMethod(getClass(), actionName, params);
            return method.invoke(this, params);

        } catch (InvocationTargetException e) {
            throw new MBeanException(e);
            
        } catch (Exception e) {
            throw new ReflectionException(e);
        }
    }
}
