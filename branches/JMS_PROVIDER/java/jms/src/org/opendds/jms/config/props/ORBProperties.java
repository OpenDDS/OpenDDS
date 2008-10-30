/*
 * $Id$
 */

package org.opendds.jms.config.props;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface ORBProperties {
    String ORB_LISTEN_ENDPOINTS_ATTR = "ORBListenEndpoints";
    String ORB_DEBUG_LEVEL_ATTR = "ORBDebugLevel";
    String ORB_LOG_FILE_ATTR = "ORBLogFile";
    String ORB_ARGS_ATTR = "ORBArgs";

    String getORBListenEndpoints();

    void setORBListenEndpoints(String value);

    Integer getORBDebugLevel();

    void setORBDebugLevel(Integer value);

    String getORBLogFile();

    void setORBLogFile(String value);

    String getORBArgs();

    void setORBArgs(String value);
}
