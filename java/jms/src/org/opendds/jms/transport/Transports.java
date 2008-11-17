/*
 * $Id$
 */

package org.opendds.jms.transport;

import java.util.Properties;

import OpenDDS.DCPS.transport.TheTransportFactory;
import OpenDDS.DCPS.transport.TransportConfiguration;

import org.opendds.jms.common.SvcConfDirective;
import org.opendds.jms.common.beans.BeanHelper;
import org.opendds.jms.common.util.Serial;
import org.opendds.jms.transport.spi.Transport;
import org.opendds.jms.transport.spi.TransportRegistry;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class Transports {
    private static final Serial serial = new Serial();
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

    public static TransportConfiguration createConfiguration(String transportType, Properties properties) {
        TransportConfiguration configuration;

        synchronized (serial) {
            if (serial.overflowed()) {
                throw new IllegalStateException("Insufficient transport ids available!");
            }
            configuration = TheTransportFactory.get_or_create_configuration(serial.next(), transportType);
        }

        BeanHelper helper = new BeanHelper(configuration.getClass());
        helper.setProperties(configuration, properties);

        return configuration;
    }

    //

    private Transports() {}
}
