/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.transport;

import java.util.Properties;

import javax.jms.JMSException;

import org.opendds.jms.common.beans.BeanHelper;
import org.opendds.jms.common.lang.Strings;
import org.opendds.jms.common.util.Logger;
import org.opendds.jms.common.util.PropertiesHelper;
import org.opendds.jms.common.util.Serial;

import OpenDDS.DCPS.transport.TheTransportRegistry;
import OpenDDS.DCPS.transport.TransportConfig;
import OpenDDS.DCPS.transport.TransportInst;

/**
 * @author Steven Stallion
 */
public class TransportFactory {
    private static final Serial serial = new Serial();

    private String type;
    private Properties properties;

    public TransportFactory(String type, String value) {
        this(type, PropertiesHelper.valueOf(value));
    }

    public TransportFactory(String type, Properties properties) {
        assert type != null;
        assert properties != null;

        this.type = type;
        this.properties = properties;
    }

    protected TransportInst createConfiguration() throws JMSException {
    	TransportInst configuration;

        synchronized (serial) {
            if (serial.overflowed()) {
                throw new JMSException("Insufficient Transport IDs available");
            }
            configuration = TheTransportRegistry.create_inst("" + serial.next(), type);
        }

        Logger logger = Transports.getLogger(configuration);
        if (logger.isDebugEnabled()) {
            logger.debug("Configuring %s with %s", configuration, Strings.asIdentity(properties));
        }

        if (!properties.isEmpty()) {
            BeanHelper helper = new BeanHelper(configuration.getClass());
            helper.setProperties(configuration, properties);
        }

        return configuration;
    }

    public TransportConfig createTransport() throws JMSException {
    	TransportInst configuration = createConfiguration();

    	TransportConfig transport = TheTransportRegistry.create_config(configuration.getName());
        if (transport == null) {
            throw new JMSException("Unable to create Transport; please check logs");
        }

        transport.addLast(configuration);

        Logger logger = Transports.getLogger(configuration);
        logger.debug("Created %s", transport);

        return transport;
    }
}
