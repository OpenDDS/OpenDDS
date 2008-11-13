/*
 * $Id$
 */

package org.opendds.jms.common.util;

import java.beans.BeanInfo;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.lang.annotation.Annotation;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class BeanHelper {
    private BeanInfo beanInfo;

    private Map<String, PropertyDescriptor> properties =
        new HashMap<String, PropertyDescriptor>();

    public BeanHelper(Class clazz) {
        try {
            beanInfo = Introspector.getBeanInfo(clazz);

            for (PropertyDescriptor property : beanInfo.getPropertyDescriptors()) {
                properties.put(property.getName(), property);
            }

        } catch (Exception e) {
            throw new IllegalArgumentException(e);
        }
    }

    public BeanInfo getBeanInfo() {
        return beanInfo;
    }

    public boolean hasProperty(String name) {
        return properties.containsKey(name);
    }

    public Collection<PropertyDescriptor> getProperties() {
        return Collections.unmodifiableCollection(properties.values());
    }

    public Collection<PropertyDescriptor> findAnnotatedProperties(Class<? extends Annotation> annotationClass) {
        Collection<PropertyDescriptor> collection = new ArrayList<PropertyDescriptor>();

        for (PropertyDescriptor property : properties.values()) {
            Method method = property.getReadMethod();
            if (method != null && method.isAnnotationPresent(annotationClass)) {
                collection.add(property);

            } else {
                method = property.getWriteMethod();
                if (method != null && method.isAnnotationPresent(annotationClass)) {
                    collection.add(property);
                }
            }
        }

        return collection;
    }
}
