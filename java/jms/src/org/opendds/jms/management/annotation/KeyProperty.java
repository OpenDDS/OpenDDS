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
 * Indicates an ObjectName key property. This type should be applied
 * to mutator (setter) methods only.
 *
 * @author  Steven Stallion
 *
 * @see     javax.management.ObjectName
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
public @interface KeyProperty {
    /** Indicates if the given property is required */
    boolean required() default true;
}
