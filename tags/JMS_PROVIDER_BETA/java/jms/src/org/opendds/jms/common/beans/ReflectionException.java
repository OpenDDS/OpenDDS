/*
 * $Id$
 */

package org.opendds.jms.common.beans;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ReflectionException extends RuntimeException {

    public ReflectionException(String message) {
        super(message);
    }

    public ReflectionException(Throwable cause) {
        super(cause);
    }

    public ReflectionException(String message, Throwable cause) {
        super(message, cause);
    }
}
