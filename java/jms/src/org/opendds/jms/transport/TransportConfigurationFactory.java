/*
 * $Id$
 */

package org.opendds.jms.transport;

import java.util.Properties;

import OpenDDS.DCPS.transport.TheTransportFactory;
import OpenDDS.DCPS.transport.TransportConfiguration;

import org.opendds.jms.common.beans.BeanHelper;
import org.opendds.jms.common.util.PropertiesHelper;
import org.opendds.jms.common.util.Serial;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TransportConfigurationFactory {
    private static final Serial serial = new Serial();

    private String transportType;

    public TransportConfigurationFactory(String transportType) {
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
}
