/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.management.annotation;

import java.lang.annotation.Documented;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Indicates an MBean attribute.  This type should be applied to
 * accessor (getter) methods only.  If readOnly is false, and the
 * accessor has an assocated mutator (setter), the attribute will
 * be registered as writable.
 *
 * @author  Steven Stallion
 *
 * @see     javax.management.MBeanAttributeInfo
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
public @interface Attribute {
    /** Indicates if the given attribute is read-only */
    boolean readOnly() default false;

    /** Indicates if the given attribute is required */
    boolean required() default false;
}
