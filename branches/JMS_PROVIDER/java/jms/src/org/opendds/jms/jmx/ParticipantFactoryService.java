/*
 * $Id$
 */

package org.opendds.jms.jmx;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.omg.CORBA.StringSeqHolder;

import DDS.DomainParticipantFactory;
import OpenDDS.DCPS.TheParticipantFactory;
import OpenDDS.DCPS.TheServiceParticipant;

import org.opendds.jms.jmx.config.Attributes;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ParticipantFactoryService implements ParticipantFactoryServiceMBean {
    private static Log log = LogFactory.getLog(ParticipantFactoryService.class);

    private boolean active;

    private DomainParticipantFactory participantFactory;

    private Attributes attributes = new Attributes();

    //

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

    public DomainParticipantFactory getDomainParticipantFactory() {
        return participantFactory;
    }

    public void start() throws Exception {
        if (active) {
            throw new IllegalStateException("DomainParticipantFactory already started!");
        }

        if (log.isInfoEnabled()) {
            log.info("Starting DomainParticipantFactory");
        }

        participantFactory =
            TheParticipantFactory.WithArgs(new StringSeqHolder(attributes.toArgs()));

        if (participantFactory == null) {
            throw new IllegalStateException("Unable to initialize DomainParticipantFactory!");
        }

        active = true;
    }

    public void stop() throws Exception {
        if (!active) {
            throw new IllegalStateException("DomainParticipantFactory already stopped!");
        }

        if (log.isInfoEnabled()) {
            log.info("Stopping DomainParticipantFactory");
        }

        TheServiceParticipant.shutdown();
        participantFactory = null;

        active = false;
    }

    public void restart() throws Exception {
        if (active) {
            stop();
        }
        start();
    }
}
