/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.transport;

import javax.jms.JMSException;

import OpenDDS.DCPS.transport.TransportImpl;

/**
 * @author Steven Stallion
 * @version $Revision$
 */
public class TransportManager {
    private TransportFactory tf;
    private TransportImpl transport;

    public TransportManager(TransportFactory tf) {
        assert tf != null;

        this.tf = tf;
    }

    protected void createTransport() throws JMSException {
        transport = tf.createTransport();
    }

    public synchronized TransportImpl getTransport() throws JMSException {
        if (transport == null) {
            createTransport();
        }
        return transport;
    }
}
