/*
 * $Id$
 */

package org.opendds.jms.management;

import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import javax.management.Attribute;
import javax.management.AttributeList;
import javax.management.AttributeNotFoundException;
import javax.management.DynamicMBean;
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

import org.opendds.jms.management.annotation.Operation;
import org.opendds.jms.util.Classes;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public abstract class DynamicMBeanSupport implements DynamicMBean, MBeanRegistration {
    protected MBeanServer server;
    protected ObjectName name;

    protected Boolean registrationDone;

    protected abstract String getDescription();

    protected abstract Attributes getAttributes();

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


    public Object getAttribute(String attribute) throws AttributeNotFoundException {
        return getAttributes().getAttribute(attribute);
    }

    public void setAttribute(Attribute attribute) throws AttributeNotFoundException, InvalidAttributeValueException {
        getAttributes().setAttribute(attribute);
    }

    public AttributeList getAttributes(String[] attributes) {
        return getAttributes().getAttributes(attributes);
    }

    public AttributeList setAttributes(AttributeList attributes) {
        return getAttributes().setAttributes(attributes);
    }

    public MBeanInfo getMBeanInfo() {
        return new MBeanInfo(getClass().getName(),
                             getDescription(),
                             getAttributeInfo(),
                             getConstructorInfo(),
                             getOperationInfo(),
                             getNotificationInfo());
    }

    protected MBeanAttributeInfo[] getAttributeInfo() {
        return getAttributes().getAttributeInfo();
    }

    protected MBeanConstructorInfo[] getConstructorInfo() {
        List<MBeanConstructorInfo> info = new ArrayList<MBeanConstructorInfo>();

        for (Constructor constructor : getClass().getConstructors()) {
            info.add(new MBeanConstructorInfo(null, constructor));
        }

        return info.toArray(new MBeanConstructorInfo[info.size()]);
    }

    protected MBeanOperationInfo[] getOperationInfo() {
        List<MBeanOperationInfo> info = new ArrayList<MBeanOperationInfo>();

        for (Method method : getClass().getMethods()) {
            if (method.isAnnotationPresent(Operation.class)) {
                info.add(new MBeanOperationInfo(null, method));
            }
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
            Method method =
                Classes.findDeclaredMethod(getClass(), actionName, params);

            return method.invoke(this, params);

        } catch (Exception e) {
            throw new ReflectionException(e);
        }
    }
}
