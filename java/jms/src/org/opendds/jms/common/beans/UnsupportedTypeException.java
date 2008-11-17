/*
 * $Id$
 */

package org.opendds.jms.common.beans;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class UnsupportedTypeException extends RuntimeException {

    public UnsupportedTypeException(Object o) {
        this(o.getClass());
    }

    public UnsupportedTypeException(Class clazz) {
        this(clazz.getName());
    }

    public UnsupportedTypeException(String message) {
        super(message);
    }
}
