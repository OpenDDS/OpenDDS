/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.management.argument;

import java.util.List;

import org.opendds.jms.common.lang.Strings;
import org.opendds.jms.management.DynamicMBeanSupport;
import org.opendds.jms.transport.Transports;

/**
 * @author  Steven Stallion
 */
public class DCPSArguments implements DynamicArgumentProvider {
    public static final String DCPS_DEBUG_LEVEL = "DCPSDebugLevel";
    public static final String DCPS_CONFIG_FILE = "DCPSConfigFile";
    public static final String DCPS_INFOREPO = "DCPSInfoRepo";
    public static final String DCPS_CHUNKS = "DCPSChunks";
    public static final String DCPS_CHUNK_MULTIPLIER = "DCPSChunkMultiplier";
    public static final String DCPS_LIVELINESS_FACTOR = "DCPSLivelinessFactor";
    public static final String DCPS_BIT = "DCPSBit";
    public static final String DCPS_BIT_TRANSPORT_ADDRESS = "DCPSBitTransportIPAddress";
    public static final String DCPS_BIT_TRANSPORT_PORT = "DCPSBitTransportPort";
    public static final String DCPS_BIT_LOOKUP_DURATION_MSEC = "DCPSBitLookupDurationMsec";
    public static final String DCPS_PENDING_TIMEOUT = "DCPSPendingTimeout";
    public static final String DCPS_PERSISTENT_DATA_DIR = "DCPSPersistentDataDir";
    public static final String DCPS_TRANSPORT_DEBUG_LEVEL = "DCPSTransportDebugLevel";
    public static final String TRANSPORT_TYPE = "TransportType";

    private DynamicMBeanSupport instance;

    public void setInstance(DynamicMBeanSupport instance) {
        assert instance != null;

        this.instance = instance;
    }

    public void registerAttributes() {
        instance.registerAttribute(DCPS_DEBUG_LEVEL, Integer.class);
        instance.registerAttribute(DCPS_CONFIG_FILE, String.class);
        instance.registerAttribute(DCPS_INFOREPO, String.class);
        instance.registerAttribute(DCPS_CHUNKS, Integer.class);
        instance.registerAttribute(DCPS_CHUNK_MULTIPLIER, Integer.class);
        instance.registerAttribute(DCPS_LIVELINESS_FACTOR, Integer.class);
        instance.registerAttribute(DCPS_BIT, Integer.class);
        instance.registerAttribute(DCPS_BIT_TRANSPORT_ADDRESS, String.class);
        instance.registerAttribute(DCPS_BIT_TRANSPORT_PORT, Integer.class);
        instance.registerAttribute(DCPS_BIT_LOOKUP_DURATION_MSEC, Integer.class);
        instance.registerAttribute(DCPS_PENDING_TIMEOUT, Integer.class);
        instance.registerAttribute(DCPS_PERSISTENT_DATA_DIR, String.class);
        instance.registerAttribute(DCPS_TRANSPORT_DEBUG_LEVEL, Integer.class);
        instance.registerAttribute(TRANSPORT_TYPE, String.class);
    }

    public void addArgs(List<String> args) throws Exception {
        ArgumentWriter writer = new ArgumentWriter(instance);

        writer.writeIfSet("-DCPSDebugLevel", DCPS_DEBUG_LEVEL);
        writer.writeIfSet("-DCPSConfigFile", DCPS_CONFIG_FILE);
        writer.writeIfSet("-DCPSInfoRepo", DCPS_INFOREPO);
        writer.writeIfSet("-DCPSChunks", DCPS_CHUNKS);
        writer.writeIfSet("-DCPSChunkMultiplier", DCPS_CHUNK_MULTIPLIER);
        writer.writeIfSet("-DCPSLivelinessFactor", DCPS_LIVELINESS_FACTOR);
        writer.writeIfSet("-DCPSBit", DCPS_BIT);
        writer.writeIfSet("-DCPSBitTransportIPAddress", DCPS_BIT_TRANSPORT_ADDRESS);
        writer.writeIfSet("-DCPSBitTransportPort", DCPS_BIT_TRANSPORT_PORT);
        writer.writeIfSet("-DCPSBitLookupDurationMsec", DCPS_BIT_LOOKUP_DURATION_MSEC);
        writer.writeIfSet("-DCPSPendingTimeout", DCPS_PENDING_TIMEOUT);
        writer.writeIfSet("-DCPSPersistentDataDir", DCPS_PERSISTENT_DATA_DIR);
        writer.writeIfSet("-DCPSTransportDebugLevel", DCPS_TRANSPORT_DEBUG_LEVEL);

        String transportType =
            (String) instance.getAttribute(TRANSPORT_TYPE);

        if (!Strings.isEmpty(transportType)) {
            SvcConfDirective directive = Transports.getDirective(transportType);
            directive.writeTo(writer);
        }

        writer.writeTo(args);
    }
}
