/*
 * $Id$
 */

package org.opendds.jms.config.spi.impl;

import java.util.List;

import org.opendds.jms.config.Configuration;
import org.opendds.jms.config.PropertyWriter;
import org.opendds.jms.config.props.ORBProperties;
import org.opendds.jms.config.spi.PropertyFormat;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ORBPropertyFormat extends PropertyFormat {

    protected void format(Configuration config, List<String> args) {
        PropertyWriter writer = new PropertyWriter(config);

        writer.writeIfSet("-ORBListenEndpoints",
            ORBProperties.ORB_LISTEN_ENDPOINTS_ATTR);

        writer.writeIfSet("-ORBDebugLevel",
            ORBProperties.ORB_DEBUG_LEVEL_ATTR);

        writer.writeIfSet("-ORBLogFile",
            ORBProperties.ORB_LOG_FILE_ATTR);

        writer.writeDelimited(ORBProperties.ORB_ARGS_ATTR);

        writer.writeTo(args);
    }
}
