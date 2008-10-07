/*
 * $Id$
 */

package org.opendds.jms;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ConnectionFactoryImpl implements ConnectionFactory {

    public Connection createConnection() throws JMSException {
        return null;
    }

    public Connection createConnection(String userName, String password) throws JMSException {
        throw new UnsupportedOperationException();
    }
}
