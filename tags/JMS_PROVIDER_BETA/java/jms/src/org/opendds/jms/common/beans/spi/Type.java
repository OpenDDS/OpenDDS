/*
 * $Id$
 */

package org.opendds.jms.common.beans.spi;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface Type<T> {

    Class[] supportedTypes();

    T defaultValue();

    T valueOf(Object o);
}
