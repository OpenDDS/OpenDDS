/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.beans.spi;

/**
 * @author  Steven Stallion
 */
public interface Type<T> {

    Class[] supportedTypes();

    T defaultValue();

    T valueOf(Object o);
}
