/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.management.argument;

import java.util.List;

import org.opendds.jms.management.DynamicMBeanSupport;

/**
 * @author  Steven Stallion
 */
public class ORBArguments implements DynamicArgumentProvider {
    public static final String ORB_LISTEN_ENDPOINTS = "ORBListenEndpoints";
    public static final String ORB_DEBUG_LEVEL = "ORBDebugLevel";
    public static final String ORB_DIRECTIVES = "ORBDirectives";
    public static final String ORB_DISABLE_NESTED_UPCALLS = "ORBDisableNestedUpcalls";
    public static final String ORB_LOG_FILE = "ORBLogFile";
    public static final String ORB_ARGS = "ORBArgs";

    private DynamicMBeanSupport instance;

    public void setInstance(DynamicMBeanSupport instance) {
        assert instance != null;

        this.instance = instance;
    }

    public void registerAttributes() {
        instance.registerAttribute(ORB_LISTEN_ENDPOINTS, String.class);
        instance.registerAttribute(ORB_DEBUG_LEVEL, Integer.class);
        instance.registerAttribute(ORB_DIRECTIVES, String.class);
        instance.registerAttribute(ORB_DISABLE_NESTED_UPCALLS, Boolean.class);
        instance.registerAttribute(ORB_LOG_FILE, String.class);
        instance.registerAttribute(ORB_ARGS, String.class);
    }

    public void addArgs(List<String> args) throws Exception {
        ArgumentWriter writer = new ArgumentWriter(instance);

        writer.writeIfSet("-ORBListenEndpoints", ORB_LISTEN_ENDPOINTS);
        writer.writeIfSet("-ORBDebugLevel", ORB_DEBUG_LEVEL);
        writer.writeIfSet("-ORBLogFile", ORB_LOG_FILE);

        writer.writeDelimitedIfSet(ORB_ARGS);
        writer.writeMultiLineIfSet(SvcConfDirective.ARGUMENT_NAME, ORB_DIRECTIVES);

        Boolean disableNestedUpcalls =
            (Boolean) instance.getAttribute(ORB_DISABLE_NESTED_UPCALLS);

        if (disableNestedUpcalls != null && disableNestedUpcalls) {
            SvcConfDirective directive;

            directive = new SvcConfDirective();
            directive.setServiceName("Client_Strategy_Factory");
            directive.addOptions("-ORBWaitStrategy", "rw");
            directive.addOptions("-ORBTransportMuxStrategy", "exclusive");
            directive.addOptions("-ORBConnectStrategy", "blocked");
            directive.addOptions("-ORBConnectionHandlerCleanup", "1");
            directive.writeTo(writer);

            directive = new SvcConfDirective();
            directive.setServiceName("Resource_Factory");
            directive.addOptions("-ORBFlushingStrategy", "blocking");
            directive.writeTo(writer);
        }

        writer.writeTo(args);
    }
}
