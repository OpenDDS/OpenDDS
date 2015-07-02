/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.lang;

import java.lang.annotation.Annotation;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.util.LinkedHashSet;
import java.util.Set;

/**
 * @author  Steven Stallion
 */
public class Annotations {

    public static boolean isAnnotated(Method method, Class<? extends Annotation> annotationClass) {
        assert annotationClass != null;

        return method != null && method.isAnnotationPresent(annotationClass);
    }

    public static <A extends Annotation> A getAnnotation(Object obj, Class<A> annotationClass) {
        assert obj != null;
        assert annotationClass != null;

        return obj.getClass().getAnnotation(annotationClass);
    }

    @SuppressWarnings("unchecked")
    public static <A extends Annotation> A getAnnotation(Constructor constructor,
                                                         Class<A> annotationClass) {
        assert constructor != null;
        assert annotationClass != null;

        return (A) constructor.getAnnotation(annotationClass);
    }

    public static <A extends Annotation> A getAnnotation(Method method,
                                                         Class<A> annotationClass) {

        assert method != null;
        assert annotationClass != null;

        return method.getAnnotation(annotationClass);
    }

    public static Set<Constructor> findAnnotatedConstructors(Object obj,
                                                             Class<? extends Annotation> annotationClass) {
        assert obj != null;

        return findAnnotatedConstructors(obj.getClass(), annotationClass);
    }

    public static Set<Constructor> findAnnotatedConstructors(Class clazz,
                                                             Class<? extends Annotation> annotationClass) {
        assert clazz != null;
        assert annotationClass != null;

        Set<Constructor> constructors = new LinkedHashSet<Constructor>();

        for (Constructor constructor : clazz.getConstructors()) {
            if (constructor.isAnnotationPresent(annotationClass)) {
                constructors.add(constructor);
            }
        }

        return constructors;
    }

    public static Set<Method> findAnnotatedMethods(Object obj,
                                                   Class<? extends Annotation> annotationClass) {
        assert obj != null;

        return findAnnotatedMethods(obj.getClass(), annotationClass);
    }

    public static Set<Method> findAnnotatedMethods(Class clazz,
                                                   Class<? extends Annotation> annotationClass) {
        assert clazz != null;
        assert annotationClass != null;

        Set<Method> methods = new LinkedHashSet<Method>();

        for (Method method : clazz.getMethods()) {
            if (method.isAnnotationPresent(annotationClass)) {
                methods.add(method);
            }
        }

        return methods;
    }

    //

    private Annotations() {}
}
