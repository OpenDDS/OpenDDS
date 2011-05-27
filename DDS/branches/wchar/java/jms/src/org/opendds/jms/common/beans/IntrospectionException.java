/*
 * $Id$
 */

package org.opendds.jms.common.beans;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class IntrospectionException extends RuntimeException {

    public IntrospectionException(String message) {
        super(message);
    }

    public IntrospectionException(Throwable cause) {
        super(cause);
    }

    public IntrospectionException(String message, Throwable cause) {
        super(message, cause);
    }
}
