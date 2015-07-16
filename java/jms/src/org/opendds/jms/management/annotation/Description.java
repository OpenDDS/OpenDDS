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
 * Indicates a human readable description.  This type may be
 * applied to elements annotated with the {@code Attribute},
 * {@code Constructor}, and {@code Operation} annotation types.
 * This type may also be applied to the MBean class itself.
 *
 * @author  Steven Stallion
 *
 * @see     Attribute
 * @see     Constructor
 * @see     Operation
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target({ ElementType.CONSTRUCTOR, ElementType.METHOD, ElementType.TYPE })
public @interface Description {
    String value();
}
