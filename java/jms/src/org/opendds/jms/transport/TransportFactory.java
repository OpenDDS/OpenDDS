/*
 * $Id$
 */

package org.opendds.jms.transport;

import java.util.Properties;

import javax.resource.ResourceException;
import javax.resource.spi.IllegalStateException;

import OpenDDS.DCPS.transport.TheTransportFactory;
import OpenDDS.DCPS.transport.TransportConfiguration;
import OpenDDS.DCPS.transport.TransportImpl;

import org.opendds.jms.common.beans.BeanHelper;
import org.opendds.jms.common.util.ContextLog;
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

    public TransportImpl createTransport(String value) throws ResourceException {
        return createTransport(PropertiesHelper.valueOf(value));
    }

    public TransportImpl createTransport(Properties properties) throws ResourceException {
        TransportConfiguration configuration;
        synchronized (serial) {
            if (serial.overflowed()) {
                throw new IllegalStateException("Insufficient Transport IDs available!");
            }
            configuration = TheTransportFactory.get_or_create_configuration(serial.next(), transportType);
        }

        ContextLog log = new ContextLog(transportType, configuration.getId()); 

        log.debug("Configuring %s %s", configuration, PropertiesHelper.valueOf(properties));

        if (!properties.isEmpty()) {
            BeanHelper helper = new BeanHelper(configuration.getClass());
            helper.setProperties(configuration, properties);
        }

        TransportImpl transport = TheTransportFactory.create_transport_impl(configuration.getId(), false);
        if (transport == null) {
            throw new ResourceException("Unable to create Transport; please check logs");
        }
        transport.configure(configuration);

        log.debug("Created %s", transport);

        return transport;
    }
}
