/*
 * $Id$
 */

package org.opendds.jms.jmx.config.spi.impl;

import java.util.List;

import org.opendds.jms.jmx.DCPSAttributes;
import org.opendds.jms.jmx.config.AttributeWriter;
import org.opendds.jms.jmx.config.Attributes;
import org.opendds.jms.jmx.config.SvcConfDirective;
import org.opendds.jms.jmx.config.TransportTypeHelper;
import org.opendds.jms.jmx.config.spi.AttributeFormat;
import org.opendds.jms.util.Strings;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class DCPSAttributeFormat extends AttributeFormat {

    protected void format(Attributes attributes, List<String> args) {
        AttributeWriter writer = new AttributeWriter(attributes);

        writer.writeIfSet("-DCPSDebugLevel",
            DCPSAttributes.DCPS_DEBUG_LEVEL_ATTR);

        writer.writeIfSet("-DCPSConfigFile",
            DCPSAttributes.DCPS_CONFIG_FILE_ATTR);

        writer.writeIfSet("-DCPSInfoRepo",
            DCPSAttributes.DCPS_INFOREPO_ATTR);

        writer.writeIfSet("-DCPSChunks",
            DCPSAttributes.DCPS_CHUNKS_ATTR);

        writer.writeIfSet("-DCPSChunkMultiplier",
            DCPSAttributes.DCPS_CHUNK_MULTIPLIER_ATTR);

        writer.writeIfSet("-DCPSLivelinessFactor",
            DCPSAttributes.DCPS_LIVELINESS_FACTOR_ATTR);

        writer.writeIfSet("-DCPSBit",
            DCPSAttributes.DCPS_BIT_ATTR);

        writer.writeIfSet("-DCPSBitTransportIPAddress",
            DCPSAttributes.DCPS_BIT_TRANSPORT_ADDRESS_ATTR);

        writer.writeIfSet("-DCPSBitTransportPort",
            DCPSAttributes.DCPS_BIT_TRANSPORT_PORT_ATTR);

        writer.writeIfSet("-DCPSBitLookupDurationMsec",
            DCPSAttributes.DCPS_BIT_LOOKUP_DURATION_MSEC_ATTR);

        writer.writeIfSet("-DCPSTransportDebugLevel",
            DCPSAttributes.DCPS_TRANSPORT_DEBUG_LEVEL_ATTR);

        //

        String transportType =
            attributes.get(DCPSAttributes.DCPS_TRANSPORT_TYPE_ATTR);

        if (!Strings.isEmpty(transportType)) {
            TransportTypeHelper helper = new TransportTypeHelper(transportType);
            for (SvcConfDirective directive : helper.getSvcConfDirectives()) {
                writer.write(directive);
            }
        }

        writer.writeTo(args);
    }
}
