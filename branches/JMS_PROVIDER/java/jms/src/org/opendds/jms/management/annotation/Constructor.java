/*
 * $Id$
 */

package org.opendds.jms.management.annotation;

import java.lang.annotation.Documented;
import java.lang.annotation.Target;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Indicates an MBean constructor.
 *
 * @author  Steven Stallion
 * @version $Revision$
 *
 * @see     javax.management.MBeanConstructorInfo
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.CONSTRUCTOR)
public @interface Constructor {}
