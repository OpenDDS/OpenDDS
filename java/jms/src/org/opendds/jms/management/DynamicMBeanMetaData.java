/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.management;

import java.beans.PropertyDescriptor;
import java.io.Serializable;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import javax.management.DynamicMBean;
import javax.management.IntrospectionException;
import javax.management.MBeanAttributeInfo;
import javax.management.MBeanConstructorInfo;
import javax.management.MBeanOperationInfo;

import org.opendds.jms.common.beans.BeanHelper;
import org.opendds.jms.common.lang.Annotations;
import org.opendds.jms.common.lang.Strings;
import org.opendds.jms.management.annotation.Attribute;
import org.opendds.jms.management.annotation.Description;
import org.opendds.jms.management.annotation.KeyProperty;
import org.opendds.jms.management.annotation.Operation;

/**
 * @author  Steven Stallion
 */
public class DynamicMBeanMetaData implements Serializable {
    private Map<String, KeyPropertyModel> properties =
        new LinkedHashMap<String, KeyPropertyModel>();

    private Map<String, AttributeModel> attributes =
        new LinkedHashMap<String, AttributeModel>();

    private List<ConstructorModel> constructors =
        new ArrayList<ConstructorModel>();

    private Map<String, OperationModel> operations =
        new LinkedHashMap<String, OperationModel>();

    private BeanHelper helper;

    /**
     * @throws  NullPointerException if instance is null
     */
    public DynamicMBeanMetaData(DynamicMBean instance) {
        this(instance.getClass());
    }

    public DynamicMBeanMetaData(Class<? extends DynamicMBean> clazz) {
        helper = new BeanHelper(clazz);

        // ObjectName Key Properties
        for (PropertyDescriptor property : helper.findAnnotatedDescriptors(KeyProperty.class)) {
            KeyPropertyModel model = new KeyPropertyModel(property);
            properties.put(model.name, model);
        }

        // MBean DynamicAttributes
        for (PropertyDescriptor property : helper.findAnnotatedDescriptors(Attribute.class)) {
            AttributeModel model = new AttributeModel(property);
            attributes.put(model.name, model);
        }

        // MBean Constructors
        for (Constructor constructor : Annotations.findAnnotatedConstructors(clazz,
            org.opendds.jms.management.annotation.Constructor.class)) {

            ConstructorModel model = new ConstructorModel(constructor);
            constructors.add(model);
        }

        // MBean Operations
        for (Method method : Annotations.findAnnotatedMethods(clazz, Operation.class)) {
            OperationModel model = new OperationModel(method);
            operations.put(model.name, model);
        }
    }

    public String getDescription(DynamicMBean instance) {
        return findDescription(instance);
    }

    public boolean hasKeyProperty(String name) {
        return properties.containsKey(name);
    }

    public Collection<KeyPropertyModel> getKeyProperties() {
        return Collections.unmodifiableCollection(properties.values());
    }

    public KeyPropertyModel getKeyProperty(String name) {
        return properties.get(name);
    }

    public boolean hasAttribute(String name) {
        return attributes.containsKey(name);
    }

    public Collection<AttributeModel> getAttributes() {
        return Collections.unmodifiableCollection(attributes.values());
    }

    public AttributeModel getAttribute(String name) {
        return attributes.get(name);
    }

    public Collection<ConstructorModel> getConstructors() {
        return Collections.unmodifiableCollection(constructors);
    }

    public boolean hasOperation(String name) {
        return operations.containsKey(name);
    }

    public Collection<OperationModel> getOperations() {
        return Collections.unmodifiableCollection(operations.values());
    }

    public OperationModel getOperation(String name) {
        return operations.get(name);
    }

    //

    public class KeyPropertyModel {
        private PropertyDescriptor descriptor;
        private String name;
        private boolean required;

        protected KeyPropertyModel(PropertyDescriptor descriptor) {
            assert descriptor != null;

            this.descriptor = descriptor;
            this.name = descriptor.getName();

            KeyProperty keyProperty =
                Annotations.getAnnotation(descriptor.getWriteMethod(), KeyProperty.class);

            this.required = keyProperty.required();
        }

        public String getName() {
            return name;
        }

        public boolean isRequired() {
            return required;
        }

        public void setValue(DynamicMBean instance, Object value) {
            helper.setProperty(instance, descriptor, value);
        }
    }

    public class AttributeModel {
        private PropertyDescriptor descriptor;
        private String name;
        private String description;
        private boolean readOnly;
        private boolean required;

        protected AttributeModel(PropertyDescriptor descriptor) {
            assert descriptor != null;

            this.descriptor = descriptor;
            this.name = Strings.capitalize(descriptor.getName());

            Method method = descriptor.getReadMethod();

            this.description = findDescription(method);

            Attribute attribute =
                Annotations.getAnnotation(method, Attribute.class);

            this.readOnly = attribute.readOnly();
            this.required = attribute.required();
        }

        public String getName() {
            return name;
        }

        public String getDescription() {
            return description;
        }

        public boolean isRequired() {
            return required;
        }

        public boolean isReadOnly() {
            return readOnly;
        }

        public Object getValue(DynamicMBean instance) {
            return helper.getProperty(instance, descriptor);
        }

        public void setValue(DynamicMBean instance, Object value) {
            helper.setProperty(instance, descriptor, value);
        }

        public MBeanAttributeInfo toAttributeInfo() {
            try {
                return new MBeanAttributeInfo(name, description, descriptor.getReadMethod(),
                    (readOnly ? null : descriptor.getWriteMethod()));

            } catch (IntrospectionException e) {
                throw new IllegalStateException(e);
            }
        }
    }

    public class ConstructorModel {
        private String description;
        private Constructor constructor;

        public ConstructorModel(Constructor constructor) {
            assert constructor != null;

            this.constructor = constructor;
            this.description = findDescription(constructor);
        }

        public String getDescription() {
            return description;
        }

        public MBeanConstructorInfo toConstructorInfo() {
            return new MBeanConstructorInfo(description, constructor);
        }
    }

    public class OperationModel {
        private String name;
        private String description;
        private Method method;

        public OperationModel(Method method) {
            assert method != null;

            this.method = method;
            this.name = method.getName();

            this.description = findDescription(method);
        }

        public String getName() {
            return name;
        }

        public String getDescription() {
            return description;
        }

        public Object invoke(DynamicMBean instance, Object[] params) throws Exception {
            return method.invoke(instance, params);
        }

        public MBeanOperationInfo toOperationInfo() {
            return new MBeanOperationInfo(description, method);
        }
    }

    //

    private static <T> String findDescription(T t) {
        String value = null;

        Description description = Annotations.getAnnotation(t, Description.class);
        if (description != null) {
            value = description.value();
        }
        return value;
    }
}
