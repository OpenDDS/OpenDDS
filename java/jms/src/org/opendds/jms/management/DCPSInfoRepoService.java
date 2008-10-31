/*
 * $Id$
 */

package org.opendds.jms.management;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.opendds.jms.DCPSInfoRepo;
import org.opendds.jms.config.Configuration;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class DCPSInfoRepoService implements DCPSInfoRepoServiceMBean {
    private static Log log = LogFactory.getLog(DCPSInfoRepoService.class);

    private boolean active;

    private DCPSInfoRepo instance;
    private Thread instanceThread;

    private Configuration config = new Configuration();

    //

    public String getBitListenAddress() {
        return config.get(BIT_LISTEN_ADDRESS);
    }

    public void setBitListenAddress(String value) {
        config.set(BIT_LISTEN_ADDRESS, value);
    }

    public String getIORFile() {
        return config.get(IOR_FILE);
    }

    public void setIORFile(String value) {
        config.set(IOR_FILE, value);
    }

    public Boolean getNOBITS() {
        return config.get(NOBITS);
    }

    public void setNOBITS(Boolean value) {
        config.set(NOBITS, value);
    }

    public Boolean getVerboseTransportLogging() {
        return config.get(VERBOSE_TRANSPORT_LOGGING);
    }

    public void setVerboseTransportLogging(Boolean value) {
        config.set(VERBOSE_TRANSPORT_LOGGING, value);
    }

    public String getPersistentFile() {
        return config.get(PERSISTENT_FILE);
    }

    public void setPersistentFile(String value) {
        config.set(PERSISTENT_FILE, value);
    }

    public Boolean getResurrectFromFile() {
        return config.get(RESURRECT_FROM_FILE);
    }

    public void setResurrectFromFile(Boolean value) {
        config.set(RESURRECT_FROM_FILE, value);
    }

    public String getFederatorConfig() {
        return config.get(FEDERATOR_CONFIG);
    }

    public void setFederatorConfig(String value) {
        config.set(FEDERATOR_CONFIG, value);
    }

    public String getFederationId() {
        return config.get(FEDERATION_ID);
    }

    public void setFederationId(String value) {
        config.set(FEDERATION_ID, value);
    }

    public String getFederateWith() {
        return config.get(FEDERATE_WITH);
    }

    public void setFederateWith(String value) {
        config.set(FEDERATE_WITH, value);
    }

    public Integer getDCPSDebugLevel() {
        return config.get(DCPS_DEBUG_LEVEL);
    }

    public void setDCPSDebugLevel(Integer value) {
        config.set(DCPS_DEBUG_LEVEL, value);
    }

    public String getDCPSConfigFile() {
        return config.get(DCPS_CONFIG_FILE);
    }

    public void setDCPSConfigFile(String value) {
        config.set(DCPS_CONFIG_FILE, value);
    }

    public String getDCPSInfoRepo() {
        return config.get(DCPS_INFOREPO);
    }

    public void setDCPSInfoRepo(String value) {
        config.set(DCPS_INFOREPO, value);
    }

    public Integer getDCPSChunks() {
        return config.get(DCPS_CHUNKS);
    }

    public void setDCPSChunks(Integer value) {
        config.set(DCPS_CHUNKS, value);
    }

    public Integer getDCPSChunkMultiplier() {
        return config.get(DCPS_CHUNK_MULTIPLIER);
    }

    public void setDCPSChunkMultiplier(Integer value) {
        config.set(DCPS_CHUNK_MULTIPLIER, value);
    }

    public Integer getDCPSLivelinessFactor() {
        return config.get(DCPS_LIVELINESS_FACTOR);
    }

    public void setDCPSLivelinessFactor(Integer value) {
        config.set(DCPS_LIVELINESS_FACTOR, value);
    }

    public Integer getDCPSBit() {
        return config.get(DCPS_BIT);
    }

    public void setDCPSBit(Integer value) {
        config.set(DCPS_BIT, value);
    }

    public String getDCPSBitTransportIPAddress() {
        return config.get(DCPS_BIT_TRANSPORT_ADDRESS);
    }

    public void setDCPSBitTransportIPAddress(String value) {
        config.set(DCPS_BIT_TRANSPORT_ADDRESS, value);
    }

    public Integer getDCPSBitTransportPort() {
        return config.get(DCPS_BIT_TRANSPORT_PORT);
    }

    public void setDCPSBitTransportPort(Integer value) {
        config.set(DCPS_BIT_TRANSPORT_PORT, value);
    }

    public Integer getDCPSBitLookupDurationMsec() {
        return config.get(DCPS_BIT_LOOKUP_DURATION_MSEC);
    }

    public void setDCPSBitLookupDurationMsec(Integer value) {
        config.set(DCPS_BIT_LOOKUP_DURATION_MSEC, value);
    }

    public Integer getDCPSTransportDebugLevel() {
        return config.get(DCPS_TRANSPORT_DEBUG_LEVEL);
    }

    public void setDCPSTransportDebugLevel(Integer value) {
        config.set(DCPS_TRANSPORT_DEBUG_LEVEL, value);
    }

    public String getDCPSTransportType() {
        return config.get(DCPS_TRANSPORT_TYPE);
    }

    public void setDCPSTransportType(String value) {
        config.set(DCPS_TRANSPORT_TYPE, value);
    }

    public String getORBListenEndpoints() {
        return config.get(ORB_LISTEN_ENDPOINTS);
    }

    public void setORBListenEndpoints(String value) {
        config.set(ORB_LISTEN_ENDPOINTS, value);
    }

    public Integer getORBDebugLevel() {
        return config.get(ORB_DEBUG_LEVEL);
    }

    public void setORBDebugLevel(Integer value) {
        config.set(ORB_DEBUG_LEVEL, value);
    }

    public String getORBLogFile() {
        return config.get(ORB_LOG_FILE);
    }

    public void setORBLogFile(String value) {
        config.set(ORB_LOG_FILE, value);
    }

    public String getORBArgs() {
        return config.get(ORB_ARGS);
    }

    public void setORBArgs(String value) {
        config.set(ORB_ARGS, value);
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
            log.info("Starting DCPSInfoRepoService");
        }

        instance = new DCPSInfoRepo(config.toArgs());

        instanceThread = new Thread(instance, "DCPSInfoRepo");
        instanceThread.start();

        active = true;
    }

    public void stop() throws Exception {
        if (!isActive()) {
            throw new IllegalStateException("DCPSInfoRepoService already stopped!");
        }

        if (log.isInfoEnabled()) {
            log.info("Stopping DCPSInfoRepoService");
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
