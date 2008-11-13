/*
 * $Id$
 */

package org.opendds.jms.transport;

import java.util.LinkedHashMap;
import java.util.Map;

import org.opendds.jms.common.SvcConfDirective;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TransportHelper {
    private static Map<String, SvcConfDirective> transports =
        new LinkedHashMap<String, SvcConfDirective>();

    static {
        SvcConfDirective directive;

        // SimpleTcp TransportType
        directive = new SvcConfDirective(true);

        directive.setServiceName("DCPS_SimpleTcpLoader");
        directive.setBaseObjectType("Service_Object *");
        directive.setLibrary("SimpleTcp");
        directive.setFactoryFunction("_make_DCPS_SimpleTcpLoader()");
        directive.addOption("-type");
        directive.addOption("SimpleTcp");

        transports.put("SimpleTcp", directive);

        // SimpleUdp TransportType
        directive = new SvcConfDirective(true);

        directive.setServiceName("OPENDDS_DCPS_SimpleUnreliableDgramLoader");
        directive.setBaseObjectType("Service_Object *");
        directive.setLibrary("SimpleUnreliableDgram");
        directive.setFactoryFunction("_make_OPENDDS_DCPS_SimpleUnreliableDgramLoader()");
        directive.addOption("-type");
        directive.addOption("SimpleUdp");

        transports.put("SimpleUdp", directive);

        // SimpleMcast TransportType
        directive = new SvcConfDirective(true);

        directive.setServiceName("OPENDDS_DCPS_SimpleUnreliableDgramLoader");
        directive.setBaseObjectType("Service_Object *");
        directive.setLibrary("SimpleUnreliableDgram");
        directive.setFactoryFunction("_make_OPENDDS_DCPS_SimpleUnreliableDgramLoader()");
        directive.addOption("-type");
        directive.addOption("SimpleMcast");

        transports.put("SimpleMcast", directive);

        // ReliableMulticast TransportType
        directive = new SvcConfDirective(true);

        directive.setServiceName("OPENDDS_DCPS_ReliableMulticastLoader");
        directive.setBaseObjectType("Service_Object *");
        directive.setLibrary("ReliableMulticast");
        directive.setFactoryFunction("_make_OPENDDS_DCPS_ReliableMulticastLoader()");

        transports.put("ReliableMulticast", directive);
    }

    public static SvcConfDirective getSvcConfDirective(String transportType) {
        SvcConfDirective directive = transports.get(transportType);
        if (directive == null) {
            throw new IllegalArgumentException("Unknown transport type: " + transportType);
        }
        return directive;
    }

    //
}
