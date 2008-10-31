/*
 * $Id$
 */

package org.opendds.jms.config.spi.impl;

import java.util.List;

import org.opendds.jms.config.Configuration;
import org.opendds.jms.config.PropertyWriter;
import org.opendds.jms.config.SvcConfDirective;
import org.opendds.jms.config.TransportTypeHelper;
import org.opendds.jms.config.properties.DCPSProperties;
import org.opendds.jms.config.spi.PropertyFormat;
import org.opendds.jms.util.Strings;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class DCPSPropertyFormat extends PropertyFormat {

    protected void format(Configuration config, List<String> args) {
        PropertyWriter writer = new PropertyWriter(config);

        writer.writeIfSet("-DCPSDebugLevel",
            DCPSProperties.DCPS_DEBUG_LEVEL);

        writer.writeIfSet("-DCPSConfigFile",
            DCPSProperties.DCPS_CONFIG_FILE);

        writer.writeIfSet("-DCPSInfoRepo",
            DCPSProperties.DCPS_INFOREPO);

        writer.writeIfSet("-DCPSChunks",
            DCPSProperties.DCPS_CHUNKS);

        writer.writeIfSet("-DCPSChunkMultiplier",
            DCPSProperties.DCPS_CHUNK_MULTIPLIER);

        writer.writeIfSet("-DCPSLivelinessFactor",
            DCPSProperties.DCPS_LIVELINESS_FACTOR);

        writer.writeIfSet("-DCPSBit",
            DCPSProperties.DCPS_BIT);

        writer.writeIfSet("-DCPSBitTransportIPAddress",
            DCPSProperties.DCPS_BIT_TRANSPORT_ADDRESS);

        writer.writeIfSet("-DCPSBitTransportPort",
            DCPSProperties.DCPS_BIT_TRANSPORT_PORT);

        writer.writeIfSet("-DCPSBitLookupDurationMsec",
            DCPSProperties.DCPS_BIT_LOOKUP_DURATION_MSEC);

        writer.writeIfSet("-DCPSTransportDebugLevel",
            DCPSProperties.DCPS_TRANSPORT_DEBUG_LEVEL);

        //

        String transportType =
            config.get(DCPSProperties.DCPS_TRANSPORT_TYPE);

        if (!Strings.isEmpty(transportType)) {
            TransportTypeHelper helper = new TransportTypeHelper(transportType);
            for (SvcConfDirective directive : helper.getSvcConfDirectives()) {
                directive.writeTo(writer);
            }
        }

        writer.writeTo(args);
    }
}
