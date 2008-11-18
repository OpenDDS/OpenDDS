/*
 * $Id$
 */

package org.opendds.jms.transport;

import org.opendds.jms.common.SvcConfDirective;
import org.opendds.jms.transport.spi.Transport;
import org.opendds.jms.transport.spi.TransportRegistry;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class Transports {
    private static TransportRegistry registry = new TransportRegistry();

    static {
        registry.registerAll();
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
