/*
 * $Id$
 */

package org.opendds.jms.management.annotation;

import java.lang.annotation.Documented;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Indicates an MBean attribute.  This type should be applied to
 * accessor (getter) methods only.
 *
 * @author  Steven Stallion
 * @version $Revision$
 *
 * @see     javax.management.MBeanAttributeInfo
 */
@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
public @interface Attribute {}
