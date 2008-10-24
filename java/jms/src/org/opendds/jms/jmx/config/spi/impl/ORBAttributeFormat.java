/*
 * $Id$
 */

package org.opendds.jms.jmx.config.spi.impl;

import java.util.List;

import org.opendds.jms.jmx.ORBAttributes;
import org.opendds.jms.jmx.config.AttributeWriter;
import org.opendds.jms.jmx.config.Attributes;
import org.opendds.jms.jmx.config.spi.AttributeFormat;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ORBAttributeFormat extends AttributeFormat {

    protected void format(Attributes attributes, List<String> args) {
        AttributeWriter writer = new AttributeWriter(attributes);

        writer.writeIfSet("-ORBListenEndpoints",
            ORBAttributes.ORB_LISTEN_ENDPOINTS_ATTR);

        writer.writeIfSet("-ORBDebugLevel",
            ORBAttributes.ORB_DEBUG_LEVEL_ATTR);

        writer.writeIfSet("-ORBLogFile",
            ORBAttributes.ORB_LOG_FILE_ATTR);

        writer.writeDelimited(ORBAttributes.ORB_ARGS_ATTR);

        writer.writeTo(args);
    }
}
