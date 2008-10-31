/*
 * $Id$
 */

package org.opendds.jms.management;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import DDS.DomainParticipantFactory;
import OpenDDS.DCPS.TheParticipantFactory;

import org.opendds.jms.config.Configuration;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ParticipantFactoryService implements ParticipantFactoryServiceMBean {
    private static Log log = LogFactory.getLog(ParticipantFactoryService.class);

    private boolean active;

    private DomainParticipantFactory instance;

    private Configuration config = new Configuration();

    //

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

    public DomainParticipantFactory getInstance() {
        return instance;
    }

    public boolean isActive() {
        return active;
    }

    public void start() throws Exception {
        if (isActive()) {
            throw new IllegalStateException("ParticipantFactoryService already started!");
        }

        if (log.isInfoEnabled()) {
            log.info("Starting ParticipantFactoryService");
        }

        instance = TheParticipantFactory.WithArgs(config.toSeqHolder());
        if (instance == null) {
            throw new IllegalStateException("Unable to initialize DomainParticipantFactory; please check logs.");
        }

        active = true;
    }

    public void stop() throws Exception {
        if (!isActive()) {
            throw new IllegalStateException("ParticipantFactoryService already stopped!");
        }

        if (log.isInfoEnabled()) {
            log.info("Stopping ParticipantFactoryService");
        }

        instance = null;

        active = false;
    }

    public void restart() throws Exception {
        if (isActive()) {
            stop();
        }
        start();
    }
}
