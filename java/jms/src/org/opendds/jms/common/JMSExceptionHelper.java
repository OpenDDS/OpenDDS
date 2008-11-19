/*
 * $
 */

package org.opendds.jms.common;

import javax.jms.JMSException;

/**
 * @author Steven Stallion
 * @version $Revision$
 */
public class JMSExceptionHelper {

    public static JMSException wrap(Exception cause) {
        JMSException e = new JMSException(cause.getMessage());
        e.setLinkedException(cause);
        return e;
    }

    //

    private JMSExceptionHelper() {}
}
