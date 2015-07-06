/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.management;

import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;

import javax.management.Attribute;
import javax.management.AttributeList;
import javax.management.AttributeNotFoundException;
import javax.management.DynamicMBean;
import javax.management.InvalidAttributeValueException;
import javax.management.JMException;
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

import org.opendds.jms.common.lang.Strings;
import org.opendds.jms.common.util.Logger;

/**
 * @author  Steven Stallion
 */
public abstract class DynamicMBeanSupport implements DynamicMBean, MBeanRegistration {
    protected MBeanServer server;
    protected ObjectName name;
    protected Boolean registrationDone;

    protected DynamicAttributes attributes = new DynamicAttributes();
    protected DynamicMBeanMetaData metadata = new DynamicMBeanMetaData(this);

    private Logger logger = Logger.getLogger(getClass());

    public ObjectName preRegister(MBeanServer server, ObjectName name) throws Exception {
        this.server = server;
        this.name = name;

        // Set ObjectName key properties
        for (DynamicMBeanMetaData.KeyPropertyModel model : metadata.getKeyProperties()) {
            String value = name.getKeyProperty(model.getName());

            if (model.isRequired() && Strings.isEmpty(value)) {
                throw new IllegalArgumentException(model.getName() + " is a required key property!");
            }
            model.setValue(this, value);
        }

        return name;
    }

    public void postRegister(Boolean registrationDone) {
        this.registrationDone = registrationDone;
    }

    public void preDeregister() {}

    public void postDeregister() {}

    public void registerAttribute(String attribute, Class type) {
        registerAttribute(attribute, null, type);
    }

    public void registerAttribute(String attribute, String property, Class type) {
        registerAttribute(attribute, property, type, null);
    }

    public void registerAttribute(String attribute, Class type, String description) {
        registerAttribute(attribute, null, type, description);
    }

    public void registerAttribute(String attribute, String property, Class type, String description) {
        attributes.register(attribute, type, description, true, true);
        if (!Strings.isEmpty(property)) {
            attributes.map(attribute, property);
        }
    }

    public void registerReadOnlyAttribute(String attribute, Class type) {
        registerReadOnlyAttribute(attribute, type, null);
    }

    public void registerReadOnlyAttribute(String attribute, Class type, String description) {
        attributes.register(attribute, type, description, true, false);
    }

    public Object getAttribute(String attribute)
            throws AttributeNotFoundException, MBeanException, ReflectionException {

        DynamicMBeanMetaData.AttributeModel model = metadata.getAttribute(attribute);
        if (model != null) {
            try {
                return model.getValue(this);

            } catch (Exception e) {
                throwException(e);
            }
        }

        // Dynamic Attribute
        return attributes.getAttribute(attribute);
    }

    public void setAttribute(Attribute attribute)
        throws AttributeNotFoundException, InvalidAttributeValueException, MBeanException, ReflectionException {

        assert attribute != null;

        String name = attribute.getName();

        DynamicMBeanMetaData.AttributeModel model = metadata.getAttribute(name);
        if (model != null) {
            if (model.isReadOnly()) {
                throw new AttributeNotFoundException(name);
            }

            try {
                model.setValue(this, attribute.getValue());
                return;

            } catch (Exception e) {
                throwException(e);
            }
        }

        attributes.setAttribute(attribute);
    }

    public AttributeList getAttributes(String[] attributes) {
        assert attributes != null;

        AttributeList list = new AttributeList();

        for (String attribute : attributes) {
            try {
                list.add(new Attribute(attribute, getAttribute(attribute)));

            } catch (Exception e) {
                logger.error("Unexpected problem getting attribute: " + attribute, e);
            }
        }

        return list;
    }

    public AttributeList setAttributes(AttributeList attributes) {
        assert attributes != null;

        AttributeList list = new AttributeList();

        Iterator itr = attributes.iterator();
        while (itr.hasNext()) {
            Attribute attribute = (Attribute) itr.next();
            try {
                setAttribute(attribute);
                list.add(attribute);

            } catch (Exception e) {
                logger.error("Unexpected problem setting attribute: " + attribute.getName(), e);
            }
        }

        return list;
    }

    public void verify() {
        for (DynamicMBeanMetaData.AttributeModel model : metadata.getAttributes()) {
            if (model.isRequired()) {
                try {
                    Object value = getAttribute(model.getName());
                    if (value == null) {
                        throw new IllegalStateException(model.getName() + " is a required attribute!");
                    }

                } catch (JMException e) {
                    throw new IllegalStateException(e);
                }
            }
        }
    }

    public Object invoke(String actionName,
                         Object params[],
                         String signature[]) throws MBeanException, ReflectionException {

        DynamicMBeanMetaData.OperationModel model = metadata.getOperation(actionName);
        if (model == null) {
            throw new IllegalArgumentException(actionName);
        }

        try {
            return model.invoke(this, params);

        } catch (Exception e) {
            throwException(e);
        }
        return null; // never reached
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
        return metadata.getDescription(this);
    }

    protected MBeanAttributeInfo[] getAttributeInfo() {
        Collection<MBeanAttributeInfo> values = new ArrayList<MBeanAttributeInfo>();

        for (DynamicMBeanMetaData.AttributeModel model : metadata.getAttributes()) {
            values.add(model.toAttributeInfo());
        }

        values.addAll(attributes.getAttributeInfo());

        return values.toArray(new MBeanAttributeInfo[values.size()]);
    }

    protected MBeanConstructorInfo[] getConstructorInfo() {
        Collection<MBeanConstructorInfo> values = new ArrayList<MBeanConstructorInfo>();

        for (DynamicMBeanMetaData.ConstructorModel model : metadata.getConstructors()) {
            values.add(model.toConstructorInfo());
        }

        return values.toArray(new MBeanConstructorInfo[values.size()]);
    }

    protected MBeanOperationInfo[] getOperationInfo() {
        Collection<MBeanOperationInfo> values = new ArrayList<MBeanOperationInfo>();

        for (DynamicMBeanMetaData.OperationModel model : metadata.getOperations()) {
            values.add(model.toOperationInfo());
        }

        return values.toArray(new MBeanOperationInfo[values.size()]);
    }

    protected MBeanNotificationInfo[] getNotificationInfo() {
        return null;
    }

    //

    private static void throwException(Exception e) throws MBeanException, ReflectionException {
        if (e instanceof InvocationTargetException) {
            Throwable cause = e.getCause();

            if (cause instanceof Exception) {
                throw new MBeanException((Exception) cause);
            }
        }
        throw new ReflectionException(e);
    }
}
