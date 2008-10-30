/*
 * $Id$
 */

package org.opendds.jms.config.props;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface DCPSProperties {
    String DCPS_DEBUG_LEVEL_ATTR = "DCPSDebugLevel";
    String DCPS_CONFIG_FILE_ATTR = "DCPSConfigFile";
    String DCPS_INFOREPO_ATTR = "DCPSInfoRepo";
    String DCPS_CHUNKS_ATTR = "DCPSChunks";
    String DCPS_CHUNK_MULTIPLIER_ATTR = "DCPSChunkMultiplier";
    String DCPS_LIVELINESS_FACTOR_ATTR = "DCPSLivelinessFactor";
    String DCPS_BIT_ATTR = "DCPSBit";
    String DCPS_BIT_TRANSPORT_ADDRESS_ATTR = "DCPSBitTransportIPAddress";
    String DCPS_BIT_TRANSPORT_PORT_ATTR = "DCPSBitTransportPort";
    String DCPS_BIT_LOOKUP_DURATION_MSEC_ATTR = "DCPSBitLookupDurationMsec";
    String DCPS_TRANSPORT_DEBUG_LEVEL_ATTR = "DCPSTransportDebugLevel";
    String DCPS_TRANSPORT_TYPE_ATTR = "DCPSTransportType";

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
