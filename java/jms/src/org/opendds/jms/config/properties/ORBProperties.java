/*
 * $Id$
 */

package org.opendds.jms.config.properties;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface ORBProperties {
    String ORB_LISTEN_ENDPOINTS = "ORBListenEndpoints";
    String ORB_DEBUG_LEVEL = "ORBDebugLevel";
    String ORB_LOG_FILE = "ORBLogFile";
    String ORB_ARGS = "ORBArgs";

    String getORBListenEndpoints();

    void setORBListenEndpoints(String value);

    Integer getORBDebugLevel();

    void setORBDebugLevel(Integer value);

    String getORBLogFile();

    void setORBLogFile(String value);

    String getORBArgs();

    void setORBArgs(String value);
}
