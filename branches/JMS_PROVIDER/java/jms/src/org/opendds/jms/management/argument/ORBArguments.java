/*
 * $Id$
 */

package org.opendds.jms.management.argument;

import java.util.List;

import org.opendds.jms.config.properties.ORBProperties;
import org.opendds.jms.management.DynamicMBeanSupport;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ORBArguments implements DynamicArgumentProvider {
    private static final String ORB_LISTEN_ENDPOINTS = "ORBListenEndpoints";
    private static final String ORB_DEBUG_LEVEL = "ORBDebugLevel";
    private static final String ORB_LOG_FILE = "ORBLogFile";
    private static final String ORB_ARGS = "ORBArgs";

    private DynamicMBeanSupport instance;

    public void setInstance(DynamicMBeanSupport instance) {
        this.instance = instance;
    }

    public void registerAttributes() {
        instance.registerAttribute(ORB_LISTEN_ENDPOINTS, String.class);
        instance.registerAttribute(ORB_DEBUG_LEVEL, Integer.class);
        instance.registerAttribute(ORB_LOG_FILE, String.class);
        instance.registerAttribute(ORB_ARGS, String.class);
    }

    public void addArgs(List<String> args) throws Exception {
        ArgumentWriter writer = new ArgumentWriter(instance);

        writer.writeIfSet("-ORBListenEndpoints", ORB_LISTEN_ENDPOINTS);
        writer.writeIfSet("-ORBDebugLevel", ORB_DEBUG_LEVEL);
        writer.writeIfSet("-ORBLogFile", ORB_LOG_FILE);

        writer.writeDelimited(ORB_ARGS);

        writer.writeTo(args);
    }
}