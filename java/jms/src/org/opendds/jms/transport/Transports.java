/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.transport;

import org.opendds.jms.common.util.Logger;
import org.opendds.jms.management.argument.SvcConfDirective;
import org.opendds.jms.transport.spi.Transport;
import org.opendds.jms.transport.spi.TransportRegistry;

import OpenDDS.DCPS.transport.TransportInst;

/**
 * @author  Steven Stallion
 */
public class Transports {
    private static TransportRegistry registry = new TransportRegistry();

    static {
        registry.registerAll();
    }

    public static Logger getLogger(TransportInst configuration) {
        assert configuration != null;

        return Logger.getLogger(configuration.getType(), configuration.getName());
    }

    public static SvcConfDirective getDirective(String transportType) {
        Transport transport = registry.findTransport(transportType);
        if (transport == null) {
            throw new IllegalArgumentException(transportType);
        }
        return transport.getDirective();
    }

    //

    private Transports() {}
}
