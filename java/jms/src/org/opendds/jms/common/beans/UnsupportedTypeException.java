/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.beans;

/**
 * @author  Steven Stallion
 */
public class UnsupportedTypeException extends RuntimeException {

    /**
     * @throws  NullPointerException if o is null
     */
    public UnsupportedTypeException(Object o) {
        this(o.getClass());
    }

    /**
     * @throws  NullPointerException if clazz is null
     */
    public UnsupportedTypeException(Class clazz) {
        this(clazz.getName());
    }

    public UnsupportedTypeException(String message) {
        super(message);
    }
}
