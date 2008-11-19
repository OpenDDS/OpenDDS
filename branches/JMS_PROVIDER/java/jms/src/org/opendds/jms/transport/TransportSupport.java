/*
 * $Id$
 */

package org.opendds.jms.transport;

import javax.resource.ResourceException;

import DDS.RETCODE_OK;
import OpenDDS.DCPS.transport.TheTransportFactory;
import OpenDDS.DCPS.transport.TransportConfiguration;
import OpenDDS.DCPS.transport.TransportImpl;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public abstract class TransportSupport {
    protected TransportImpl transport;

    public TransportImpl getTransport() {
        return transport;
    }

    protected void createTransport(TransportConfiguration config) throws ResourceException {
        transport = TheTransportFactory.create_transport_impl(config.getId(), false);
        if (transport == null) {
            throw new ResourceException("Unable to create TransportImpl; please check logs");
        }

        if (transport.configure(config) != RETCODE_OK.value) {
            throw new ResourceException("Unable to configure TransportImpl; please check logs");
        }
    }
}
