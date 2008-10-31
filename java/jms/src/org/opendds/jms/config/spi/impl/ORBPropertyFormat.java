/*
 * $Id$
 */

package org.opendds.jms.config.spi.impl;

import java.util.List;

import org.opendds.jms.config.Configuration;
import org.opendds.jms.config.PropertyWriter;
import org.opendds.jms.config.properties.ORBProperties;
import org.opendds.jms.config.spi.PropertyFormat;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ORBPropertyFormat extends PropertyFormat {

    protected void format(Configuration config, List<String> args) {
        PropertyWriter writer = new PropertyWriter(config);

        writer.writeIfSet("-ORBListenEndpoints",
            ORBProperties.ORB_LISTEN_ENDPOINTS);

        writer.writeIfSet("-ORBDebugLevel",
            ORBProperties.ORB_DEBUG_LEVEL);

        writer.writeIfSet("-ORBLogFile",
            ORBProperties.ORB_LOG_FILE);

        writer.writeDelimited(ORBProperties.ORB_ARGS);

        writer.writeTo(args);
    }
}
