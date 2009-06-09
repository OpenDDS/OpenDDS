/*
 * $Id$
 */

package org.opendds.jms.transport.spi;

import org.opendds.jms.common.spi.ServiceRegistry;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TransportRegistry extends ServiceRegistry<Transport> {

    protected Class<Transport> getProviderClass() {
        return Transport.class;
    }

    public Transport findTransport(String name) {
        assert name != null;

        for (Transport transport : providers) {
            if (name.equals(transport.getName())) {
                return transport;
            }
        }
        return null;
    }
}
