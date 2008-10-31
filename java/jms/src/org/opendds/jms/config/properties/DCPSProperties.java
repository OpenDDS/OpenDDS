/*
 * $Id$
 */

package org.opendds.jms.config.properties;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface DCPSProperties {
    String DCPS_DEBUG_LEVEL = "DCPSDebugLevel";
    String DCPS_CONFIG_FILE = "DCPSConfigFile";
    String DCPS_INFOREPO = "DCPSInfoRepo";
    String DCPS_CHUNKS = "DCPSChunks";
    String DCPS_CHUNK_MULTIPLIER = "DCPSChunkMultiplier";
    String DCPS_LIVELINESS_FACTOR = "DCPSLivelinessFactor";
    String DCPS_BIT = "DCPSBit";
    String DCPS_BIT_TRANSPORT_ADDRESS = "DCPSBitTransportIPAddress";
    String DCPS_BIT_TRANSPORT_PORT = "DCPSBitTransportPort";
    String DCPS_BIT_LOOKUP_DURATION_MSEC = "DCPSBitLookupDurationMsec";
    String DCPS_TRANSPORT_DEBUG_LEVEL = "DCPSTransportDebugLevel";
    String DCPS_TRANSPORT_TYPE = "DCPSTransportType";

    Integer getDCPSDebugLevel();

    void setDCPSDebugLevel(Integer value);

    String getDCPSConfigFile();

    void setDCPSConfigFile(String value);

    String getDCPSInfoRepo();

    void setDCPSInfoRepo(String value);

    Integer getDCPSChunks();

    void setDCPSChunks(Integer value);

    Integer getDCPSChunkMultiplier();

    void setDCPSChunkMultiplier(Integer value);

    Integer getDCPSLivelinessFactor();

    void setDCPSLivelinessFactor(Integer value);

    Integer getDCPSBit();

    void setDCPSBit(Integer value);

    String getDCPSBitTransportIPAddress();

    void setDCPSBitTransportIPAddress(String value);

    Integer getDCPSBitTransportPort();

    void setDCPSBitTransportPort(Integer value);

    Integer getDCPSBitLookupDurationMsec();

    void setDCPSBitLookupDurationMsec(Integer value);

    Integer getDCPSTransportDebugLevel();

    void setDCPSTransportDebugLevel(Integer value);

    String getDCPSTransportType();

    void setDCPSTransportType(String value);
}
