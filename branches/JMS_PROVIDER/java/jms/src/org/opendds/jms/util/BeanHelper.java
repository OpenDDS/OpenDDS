/*
 * $Id$
 */

package org.opendds.jms.util;

import java.beans.BeanInfo;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.lang.annotation.Annotation;
import java.lang.reflect.Method;
import java.util.LinkedHashSet;
import java.util.Set;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class BeanHelper {
    private BeanInfo beanInfo;

    public BeanHelper(Object instance) {
        this(instance.getClass());
    }

    public BeanHelper(Class clazz) {
        try {
            beanInfo = Introspector.getBeanInfo(clazz);

        } catch (Exception e) {
            throw new IllegalArgumentException(e);
        }
    }

    public BeanInfo getBeanInfo() {
        return beanInfo;
    }

    public Set<PropertyDescriptor> findAnnotatedProperties(Class<? extends Annotation> annotationClass) {
        Set<PropertyDescriptor> properties = new LinkedHashSet<PropertyDescriptor>();

        for (PropertyDescriptor property : beanInfo.getPropertyDescriptors()) {
            Method method = property.getReadMethod();
            if (method != null && method.isAnnotationPresent(annotationClass)) {
                properties.add(property);
            }
        }

        return properties;
    }
}
