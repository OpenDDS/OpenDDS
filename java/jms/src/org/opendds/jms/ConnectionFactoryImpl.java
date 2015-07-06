/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import java.io.Serializable;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.resource.ResourceException;
import javax.resource.spi.ConnectionManager;
import javax.resource.spi.ConnectionRequestInfo;
import javax.resource.spi.ManagedConnectionFactory;

import org.opendds.jms.common.ExceptionHelper;
import org.opendds.jms.common.util.Logger;

/**
 * @author  Steven Stallion
 */
public class ConnectionFactoryImpl implements ConnectionFactory, Serializable {
    private static Logger logger = Logger.getLogger(ConnectionFactoryImpl.class);

    private ManagedConnectionFactory mcf;
    private ConnectionManager cxManager;
    private ConnectionRequestInfo cxRequestInfo;

    public ConnectionFactoryImpl(ManagedConnectionFactory mcf,
                                 ConnectionManager cxManager,
                                 ConnectionRequestInfo cxRequestInfo) {
        assert mcf != null;
        assert cxManager != null;
        assert cxRequestInfo != null;

        this.mcf = mcf;
        this.cxManager = cxManager;
        this.cxRequestInfo = cxRequestInfo;
    }

    public boolean isManaged() {
        return cxManager != null;
    }

    public ConnectionManager getConnectionManager() {
        return cxManager;
    }

    public ConnectionRequestInfo getConnectionRequestInfo() {
        return cxRequestInfo;
    }

    public Connection createConnection() throws JMSException {
        if (!isManaged()) {
            throw new UnsupportedOperationException();
        }

        try {
            Connection conn = (Connection) cxManager.allocateConnection(mcf, cxRequestInfo);
            logger.debug("Created %s", conn);

            return conn;

        } catch (ResourceException e) {
            throw ExceptionHelper.wrap(e);
        }
    }

    public Connection createConnection(String userName, String password) throws JMSException {
        return createConnection(); // authentication not supported
    }
}
