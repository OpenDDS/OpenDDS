/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.transport;

import javax.jms.JMSException;

import OpenDDS.DCPS.transport.TransportConfig;

/**
 * @author Steven Stallion
 */
public class TransportManager {
    private TransportFactory tf;
    private TransportConfig transport;

    public TransportManager(TransportFactory tf) {
        assert tf != null;

        this.tf = tf;
    }

    protected void createTransport() throws JMSException {
        transport = tf.createTransport();
    }

    public synchronized TransportConfig getTransport() throws JMSException {
        if (transport == null) {
            createTransport();
        }
        return transport;
    }
}
