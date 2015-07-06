/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common;

import javax.jms.JMSException;

import org.opendds.jms.resource.ManagedConnectionImpl;

/**
 * @author Steven Stallion
 */
public class ExceptionHelper {

    public static JMSException notify(ManagedConnectionImpl connection, Exception cause) {
        assert connection != null;

        connection.notifyError(cause);

        if (cause instanceof JMSException) {
            return (JMSException) cause;

        } else {
            return wrap(cause);
        }
    }

    public static JMSException wrap(Exception cause) {
        JMSException e = new JMSException(cause.getMessage());
        e.setLinkedException(cause);
        return e;
    }

    //

    private ExceptionHelper() {}
}
