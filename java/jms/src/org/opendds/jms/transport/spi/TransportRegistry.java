/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.transport.spi;

import org.opendds.jms.common.spi.ServiceRegistry;

/**
 * @author  Steven Stallion
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
