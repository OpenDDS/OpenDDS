/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.beans;

import java.beans.BeanInfo;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.lang.annotation.Annotation;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;

import org.opendds.jms.common.beans.spi.Type;
import org.opendds.jms.common.beans.spi.TypeRegistry;
import org.opendds.jms.common.lang.Annotations;

/**
 * @author  Steven Stallion
 */
public class BeanHelper {
    private static TypeRegistry registry = new TypeRegistry();

    static {
        registry.registerAll();
    }

    private Class beanClass;
    private BeanInfo beanInfo;

    private Map<String, PropertyDescriptor> descriptors =
        new HashMap<String, PropertyDescriptor>();

    public BeanHelper(Class clazz) {
        assert clazz != null;

        this.beanClass = clazz;
        try {
            beanInfo = Introspector.getBeanInfo(clazz);

            for (PropertyDescriptor property : beanInfo.getPropertyDescriptors()) {
                descriptors.put(property.getName(), property);
            }

        } catch (Exception e) {
            throw new IntrospectionException(e);
        }
    }

    public Class getBeanClass() {
        return beanClass;
    }

    public BeanInfo getBeanInfo() {
        return beanInfo;
    }

    public Object newInstance() {
        try {
            return beanClass.newInstance();

        } catch (Exception e) {
            throw new ReflectionException(e);
        }
    }

    public Collection<PropertyDescriptor> findAnnotatedDescriptors(Class<? extends Annotation> annotationClass) {
        Collection<PropertyDescriptor> collection = new ArrayList<PropertyDescriptor>();

        for (PropertyDescriptor descriptor : descriptors.values()) {
            if (Annotations.isAnnotated(descriptor.getReadMethod(), annotationClass)
                || Annotations.isAnnotated(descriptor.getWriteMethod(), annotationClass)) {

                collection.add(descriptor);
            }
        }

        return collection;
    }

    public Collection<PropertyDescriptor> getDescriptors() {
        return Collections.unmodifiableCollection(descriptors.values());
    }

    public Object getProperty(Object instance, String property) {
        assert property != null;

        PropertyDescriptor descriptor = descriptors.get(property);
        if (descriptor == null) {
            throw new IntrospectionException(property);
        }
        return getProperty(instance, descriptor);
    }

    public Object getProperty(Object instance, PropertyDescriptor descriptor) {
        assert instance != null;
        assert descriptor != null;

        Method method = descriptor.getReadMethod();
        if (method == null) {
            throw new IntrospectionException(descriptor.getName() + " is a write-only property!");
        }

        try {
            return method.invoke(instance);

        } catch (Exception e) {
            throw new ReflectionException(e);
        }
    }

    public void setProperty(Object instance, String property, Object value) {
        assert property != null;

        PropertyDescriptor descriptor = descriptors.get(property);
        if (descriptor == null) {
            throw new IntrospectionException(property);
        }
        setProperty(instance, descriptor, value);
    }

    @SuppressWarnings("unchecked")
    public void setProperty(Object instance, PropertyDescriptor descriptor, Object value) {
        assert instance != null;
        assert descriptor != null;

        Method method = descriptor.getWriteMethod();
        if (method == null) {
            throw new IntrospectionException(descriptor.getName() + " is a read-only property!");
        }

        // Support automatic type conversion for non-null values:
        Class propertyType = descriptor.getPropertyType();
        if (value != null && !propertyType.isAssignableFrom(value.getClass())) {
            Type type = registry.findType(propertyType);
            if (type == null) {
                throw new UnsupportedTypeException(propertyType);
            }
            value = type.valueOf(value);
        }

        try {
            method.invoke(instance, value);

        } catch (Exception e) {
            throw new ReflectionException(e);
        }
    }

    public void setProperties(Object instance, Properties properties) {
        assert properties != null;

        Enumeration en = properties.propertyNames();
        while (en.hasMoreElements()) {
            String name = (String) en.nextElement();
            setProperty(instance, name, properties.getProperty(name));
        }
    }
}
