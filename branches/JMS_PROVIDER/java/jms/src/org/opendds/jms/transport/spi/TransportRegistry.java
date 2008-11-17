/*
 * $Id$
 */

package org.opendds.jms.transport.spi;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

import sun.misc.Service;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TransportRegistry {
    private List<Transport> transports = new ArrayList<Transport>();

    public void registerAll() {
        Iterator itr = Service.providers(Transport.class);
        while (itr.hasNext()) {
            register((Transport) itr.next());
        }
    }

    public void register(Transport transport) {
        transports.add(transport);
    }

    public void deregisterAll() {
        transports.clear();
    }

    public void deregister(Transport transport) {
        transports.remove(transport);
    }

    public Transport findTransport(String name) {
        assert name != null;

        for (Transport transport : transports) {
            if (name.equals(transport.getName())) {
                return transport;
            }
        }
        return null;
    }

    public Collection<Transport> getTransports() {
        return Collections.unmodifiableCollection(transports);
    }
}
