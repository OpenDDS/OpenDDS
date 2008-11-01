/*
 * $
 */

package org.opendds.jms.management.annotation;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
@Retention(RetentionPolicy.RUNTIME)
public @interface Operation {
    String description() default "";
}
