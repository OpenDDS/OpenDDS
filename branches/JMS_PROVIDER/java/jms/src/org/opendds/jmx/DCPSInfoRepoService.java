/*
 * $Id$
 */

package org.opendds.jmx;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.opendds.DCPSInfoRepo;
import org.opendds.config.Configuration;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class DCPSInfoRepoService implements DCPSInfoRepoServiceMBean {
    private static Log log = LogFactory.getLog(DCPSInfoRepoService.class);

    private boolean active;

    private DCPSInfoRepo instance;
    private Thread instanceThread;

    private Configuration attributes = new Configuration();

    //

    public String getBitListenAddress() {
        return attributes.get(BIT_LISTEN_ADDRESS_ATTR);
    }

    public void setBitListenAddress(String value) {
        attributes.set(BIT_LISTEN_ADDRESS_ATTR, value);
    }

    public String getIORFile() {
        return attributes.get(IOR_FILE_ATTR);
    }

    public void setIORFile(String value) {
        attributes.set(IOR_FILE_ATTR, value);
    }

    public Boolean getNOBITS() {
        return attributes.get(NOBITS_ATTR);
    }

    public void setNOBITS(Boolean value) {
        attributes.set(NOBITS_ATTR, value);
    }

    public Boolean getVerboseTransportLogging() {
        return attributes.get(VERBOSE_TRANSPORT_LOGGING_ATTR);
    }

    public void setVerboseTransportLogging(Boolean value) {
        attributes.set(VERBOSE_TRANSPORT_LOGGING_ATTR, value);
    }

    public String getPersistentFile() {
        return attributes.get(PERSISTENT_FILE_ATTR);
    }

    public void setPersistentFile(String value) {
        attributes.set(PERSISTENT_FILE_ATTR, value);
    }

    public Boolean getResurrectFromFile() {
        return attributes.get(RESURRECT_FROM_FILE_ATTR);
    }

    public void setResurrectFromFile(Boolean value) {
        attributes.set(RESURRECT_FROM_FILE_ATTR, value);
    }

    public String getFederatorConfig() {
        return attributes.get(FEDERATOR_CONFIG_ATTR);
    }

    public void setFederatorConfig(String value) {
        attributes.set(FEDERATOR_CONFIG_ATTR, value);
    }

    public String getFederationId() {
        return attributes.get(FEDERATION_ID_ATTR);
    }

    public void setFederationId(String value) {
        attributes.set(FEDERATION_ID_ATTR, value);
    }

    public String getFederateWith() {
        return attributes.get(FEDERATE_WITH_ATTR);
    }

    public void setFederateWith(String value) {
        attributes.set(FEDERATE_WITH_ATTR, value);
    }

    public Integer getDCPSDebugLevel() {
        return attributes.get(DCPS_DEBUG_LEVEL_ATTR);
    }

    public void setDCPSDebugLevel(Integer value) {
        attributes.set(DCPS_DEBUG_LEVEL_ATTR, value);
    }

    public String getDCPSConfigFile() {
        return attributes.get(DCPS_CONFIG_FILE_ATTR);
    }

    public void setDCPSConfigFile(String value) {
        attributes.set(DCPS_CONFIG_FILE_ATTR, value);
    }

    public String getDCPSInfoRepo() {
        return attributes.get(DCPS_INFOREPO_ATTR);
    }

    public void setDCPSInfoRepo(String value) {
        attributes.set(DCPS_INFOREPO_ATTR, value);
    }

    public Integer getDCPSChunks() {
        return attributes.get(DCPS_CHUNKS_ATTR);
    }

    public void setDCPSChunks(Integer value) {
        attributes.set(DCPS_CHUNKS_ATTR, value);
    }

    public Integer getDCPSChunkMultiplier() {
        return attributes.get(DCPS_CHUNK_MULTIPLIER_ATTR);
    }

    public void setDCPSChunkMultiplier(Integer value) {
        attributes.set(DCPS_CHUNK_MULTIPLIER_ATTR, value);
    }

    public Integer getDCPSLivelinessFactor() {
        return attributes.get(DCPS_LIVELINESS_FACTOR_ATTR);
    }

    public void setDCPSLivelinessFactor(Integer value) {
        attributes.set(DCPS_LIVELINESS_FACTOR_ATTR, value);
    }

    public Integer getDCPSBit() {
        return attributes.get(DCPS_BIT_ATTR);
    }

    public void setDCPSBit(Integer value) {
        attributes.set(DCPS_BIT_ATTR, value);
    }

    public String getDCPSBitTransportIPAddress() {
        return attributes.get(DCPS_BIT_TRANSPORT_ADDRESS_ATTR);
    }

    public void setDCPSBitTransportIPAddress(String value) {
        attributes.set(DCPS_BIT_TRANSPORT_ADDRESS_ATTR, value);
    }

    public Integer getDCPSBitTransportPort() {
        return attributes.get(DCPS_BIT_TRANSPORT_PORT_ATTR);
    }

    public void setDCPSBitTransportPort(Integer value) {
        attributes.set(DCPS_BIT_TRANSPORT_PORT_ATTR, value);
    }

    public Integer getDCPSBitLookupDurationMsec() {
        return attributes.get(DCPS_BIT_LOOKUP_DURATION_MSEC_ATTR);
    }

    public void setDCPSBitLookupDurationMsec(Integer value) {
        attributes.set(DCPS_BIT_LOOKUP_DURATION_MSEC_ATTR, value);
    }

    public Integer getDCPSTransportDebugLevel() {
        return attributes.get(DCPS_TRANSPORT_DEBUG_LEVEL_ATTR);
    }

    public void setDCPSTransportDebugLevel(Integer value) {
        attributes.set(DCPS_TRANSPORT_DEBUG_LEVEL_ATTR, value);
    }

    public String getDCPSTransportType() {
        return attributes.get(DCPS_TRANSPORT_TYPE_ATTR);
    }

    public void setDCPSTransportType(String value) {
        attributes.set(DCPS_TRANSPORT_TYPE_ATTR, value);
    }

    public String getORBListenEndpoints() {
        return attributes.get(ORB_LISTEN_ENDPOINTS_ATTR);
    }

    public void setORBListenEndpoints(String value) {
        attributes.set(ORB_LISTEN_ENDPOINTS_ATTR, value);
    }

    public Integer getORBDebugLevel() {
        return attributes.get(ORB_DEBUG_LEVEL_ATTR);
    }

    public void setORBDebugLevel(Integer value) {
        attributes.set(ORB_DEBUG_LEVEL_ATTR, value);
    }

    public String getORBLogFile() {
        return attributes.get(ORB_LOG_FILE_ATTR);
    }

    public void setORBLogFile(String value) {
        attributes.set(ORB_LOG_FILE_ATTR, value);
    }

    public String getORBArgs() {
        return attributes.get(ORB_ARGS_ATTR);
    }

    public void setORBArgs(String value) {
        attributes.set(ORB_ARGS_ATTR, value);
    }

    //

    public boolean isActive() {
        return active;
    }

    public void start() throws Exception {
        if (isActive()) {
            throw new IllegalStateException("DCPSInfoRepoService already started!");
        }

        if (log.isInfoEnabled()) {
            log.info("Starting DCPSInfoRepo");
        }

        instance = new DCPSInfoRepo(attributes.toArgs());

        instanceThread = new Thread(instance, "DCPSInfoRepo");
        instanceThread.start();

        active = true;
    }

    public void stop() throws Exception {
        if (!isActive()) {
            throw new IllegalStateException("DCPSInfoRepoService already stopped!");
        }

        if (log.isInfoEnabled()) {
            log.info("Stopping DCPSInfoRepo");
        }

        instance.shutdown();
        instanceThread.join();

        instance = null;
        instanceThread = null;

        active = false;
    }

    public void restart() throws Exception {
        if (isActive()) {
            stop();
        }
        start();
    }
}
