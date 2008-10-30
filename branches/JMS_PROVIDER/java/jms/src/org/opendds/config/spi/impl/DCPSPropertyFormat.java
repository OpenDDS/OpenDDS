/*
 * $Id$
 */

package org.opendds.config.spi.impl;

import java.util.List;

import org.opendds.config.Configuration;
import org.opendds.config.PropertyWriter;
import org.opendds.config.SvcConfDirective;
import org.opendds.config.TransportTypeHelper;
import org.opendds.config.props.DCPSProperties;
import org.opendds.config.spi.PropertyFormat;
import org.opendds.util.Strings;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class DCPSPropertyFormat extends PropertyFormat {

    protected void format(Configuration config, List<String> args) {
        PropertyWriter writer = new PropertyWriter(config);

        writer.writeIfSet("-DCPSDebugLevel",
            DCPSProperties.DCPS_DEBUG_LEVEL_ATTR);

        writer.writeIfSet("-DCPSConfigFile",
            DCPSProperties.DCPS_CONFIG_FILE_ATTR);

        writer.writeIfSet("-DCPSInfoRepo",
            DCPSProperties.DCPS_INFOREPO_ATTR);

        writer.writeIfSet("-DCPSChunks",
            DCPSProperties.DCPS_CHUNKS_ATTR);

        writer.writeIfSet("-DCPSChunkMultiplier",
            DCPSProperties.DCPS_CHUNK_MULTIPLIER_ATTR);

        writer.writeIfSet("-DCPSLivelinessFactor",
            DCPSProperties.DCPS_LIVELINESS_FACTOR_ATTR);

        writer.writeIfSet("-DCPSBit",
            DCPSProperties.DCPS_BIT_ATTR);

        writer.writeIfSet("-DCPSBitTransportIPAddress",
            DCPSProperties.DCPS_BIT_TRANSPORT_ADDRESS_ATTR);

        writer.writeIfSet("-DCPSBitTransportPort",
            DCPSProperties.DCPS_BIT_TRANSPORT_PORT_ATTR);

        writer.writeIfSet("-DCPSBitLookupDurationMsec",
            DCPSProperties.DCPS_BIT_LOOKUP_DURATION_MSEC_ATTR);

        writer.writeIfSet("-DCPSTransportDebugLevel",
            DCPSProperties.DCPS_TRANSPORT_DEBUG_LEVEL_ATTR);

        //

        String transportType =
            config.get(DCPSProperties.DCPS_TRANSPORT_TYPE_ATTR);

        if (!Strings.isEmpty(transportType)) {
            TransportTypeHelper helper = new TransportTypeHelper(transportType);
            for (SvcConfDirective directive : helper.getSvcConfDirectives()) {
                writer.write(directive);
            }
        }

        writer.writeTo(args);
    }
}
