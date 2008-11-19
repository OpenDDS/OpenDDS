/*
 * $Id$
 */

package org.opendds.jms.resource;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

import javax.resource.NotSupportedException;
import javax.resource.ResourceException;
import javax.resource.spi.ConnectionEventListener;
import javax.resource.spi.ConnectionRequestInfo;
import javax.resource.spi.LocalTransaction;
import javax.resource.spi.ManagedConnection;
import javax.resource.spi.ManagedConnectionMetaData;
import javax.security.auth.Subject;
import javax.transaction.xa.XAResource;

import org.opendds.jms.common.Version;
import org.opendds.jms.common.lang.Objects;

/**
 * @author Steven Stallion
 * @version $Revision$
 */
public class ManagedConnectionImpl implements ManagedConnection {
    private Subject subject;
    private ConnectionRequestInfo cxRequestInfo;

    private PrintWriter out;

    private List<ConnectionEventListener> listeners =
        new ArrayList<ConnectionEventListener>();

    public ManagedConnectionImpl(Subject subject, ConnectionRequestInfo cxRequestInfo) {
        this.subject = subject;
        this.cxRequestInfo = cxRequestInfo;
    }

    public Subject getSubject() {
        return subject;
    }

    public ConnectionRequestInfo getRequestInfo() {
        return cxRequestInfo;
    }

    public PrintWriter getLogWriter() {
        return out;
    }

    public void setLogWriter(PrintWriter out) {
        this.out = out;
    }

    public void addConnectionEventListener(ConnectionEventListener listener) {
        listeners.add(listener);
    }

    public void removeConnectionEventListener(ConnectionEventListener listener) {
        listeners.remove(listener);
    }

    public boolean matches(Subject subject, ConnectionRequestInfo requestInfo) {
        return Objects.equals(this.subject, subject)
            && Objects.equals(this.cxRequestInfo, requestInfo);
    }

    public void associateConnection(Object o) throws ResourceException {

    }

    public Object getConnection(Subject subject, ConnectionRequestInfo requestInfo) throws ResourceException {
        return null;
    }

    public void cleanup() throws ResourceException {
    }

    public void destroy() throws ResourceException {
    }

    public XAResource getXAResource() throws ResourceException {
        throw new NotSupportedException(); // transactions not supported
    }

    public LocalTransaction getLocalTransaction() throws ResourceException {
        throw new NotSupportedException(); // transactions not supported
    }

    public ManagedConnectionMetaData getMetaData() throws ResourceException {
        return new ManagedConnectionMetaData() {
            private Version version = Version.getInstance();

            public String getEISProductName() {
                return version.getProductName();
            }

            public String getEISProductVersion() {
                return version.getDDSVersion();
            }

            public int getMaxConnections() {
                return 0;
            }

            public String getUserName() throws ResourceException {
                return null; // authentication not supported
            }
        };
    }
}
