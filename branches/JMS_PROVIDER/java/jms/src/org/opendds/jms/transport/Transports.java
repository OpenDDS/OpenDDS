/*
 * $Id$
 */

package org.opendds.jms.transport;

import java.util.Properties;

import OpenDDS.DCPS.transport.TransportConfiguration;

import org.opendds.jms.common.SvcConfDirective;
import org.opendds.jms.common.beans.BeanHelper;
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

    public static Class getConfigurationClass(String transportType) {
        Transport transport = registry.findTransport(transportType);
        if (transport == null) {
            throw new IllegalArgumentException(transportType);
        }
        return transport.getConfigurationClass();
    }

    public static TransportConfiguration createConfiguration(String transportType,
                                                             Properties properties) {
        BeanHelper helper =
            new BeanHelper(getConfigurationClass(transportType));

        TransportConfiguration instance = (TransportConfiguration) helper.newInstance();
        helper.setProperties(instance, properties);

        return instance;
    }

    //

    private Transports() {}
}
