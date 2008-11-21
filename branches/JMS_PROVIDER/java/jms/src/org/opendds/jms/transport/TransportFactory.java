/*
 * $Id$
 */

package org.opendds.jms.transport;

import java.util.Properties;

import javax.resource.ResourceException;

import OpenDDS.DCPS.transport.TheTransportFactory;
import OpenDDS.DCPS.transport.TransportConfiguration;
import OpenDDS.DCPS.transport.TransportImpl;

import org.opendds.jms.common.beans.BeanHelper;
import org.opendds.jms.common.util.PropertiesHelper;
import org.opendds.jms.common.util.Serial;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TransportFactory {
    private static final Serial serial = new Serial();

    private String transportType;

    public TransportFactory(String transportType) {
        this.transportType = transportType;
    }

    public TransportConfiguration createConfiguration() {
        synchronized (serial) {
            if (serial.overflowed()) {
                throw new IllegalStateException("Insufficient transport ids available!");
            }
            return TheTransportFactory.get_or_create_configuration(serial.next(), transportType);
        }
    }

    public TransportConfiguration createConfiguration(String value) {
        return createConfiguration(PropertiesHelper.valueOf(value));
    }

    public TransportConfiguration createConfiguration(Properties properties) {
        TransportConfiguration configuration = createConfiguration();

        if (!properties.isEmpty()) {
            BeanHelper helper = new BeanHelper(configuration.getClass());
            helper.setProperties(configuration, properties);
        }

        return configuration;
    }

    public TransportImpl createTransport(String value) throws ResourceException {
        return createTransport(createConfiguration(value));
    }

    public TransportImpl createTransport(Properties properties) throws ResourceException {
        return createTransport(createConfiguration(properties));
    }

    public TransportImpl createTransport(TransportConfiguration configuration) throws ResourceException {
        TransportImpl transport =
            TheTransportFactory.create_transport_impl(configuration.getId(), false);

        if (transport == null) {
            throw new ResourceException("Unable to create Transport; please check logs");
        }

        transport.configure(configuration);
        return transport;
    }
}
