/*
 * $Id$
 */

package org.opendds.config;

import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.StringTokenizer;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class TransportTypeHelper {
    private static Map<String, SvcConfDirective> transports =
        new LinkedHashMap<String, SvcConfDirective>();

    static {
        SvcConfDirective directive;

        // SimpleTcp Transport
        directive = new SvcConfDirective(true);

        directive.setServiceName("DCPS_SimpleTcpLoader");
        directive.setBaseObjectType("Service_Object *");
        directive.setLibrary("SimpleTcp");
        directive.setFactoryFunction("_make_DCPS_SimpleTcpLoader()");
        directive.addOption("-type");
        directive.addOption("SimpleTcp");

        transports.put("SimpleTcp", directive);

        // SimpleUdp Transport
        directive = new SvcConfDirective(true);

        directive.setServiceName("OPENDDS_DCPS_SimpleUnreliableDgramLoader");
        directive.setBaseObjectType("Service_Object *");
        directive.setLibrary("SimpleUnreliableDgram");
        directive.setFactoryFunction("_make_OPENDDS_DCPS_SimpleUnreliableDgramLoader()");
        directive.addOption("-type");
        directive.addOption("SimpleUdp");

        transports.put("SimpleUdp", directive);

        // SimpleMcast Transport
        directive = new SvcConfDirective(true);

        directive.setServiceName("OPENDDS_DCPS_SimpleUnreliableDgramLoader");
        directive.setBaseObjectType("Service_Object *");
        directive.setLibrary("SimpleUnreliableDgram");
        directive.setFactoryFunction("_make_OPENDDS_DCPS_SimpleUnreliableDgramLoader()");
        directive.addOption("-type");
        directive.addOption("SimpleMcast");

        transports.put("SimpleMcast", directive);

        // ReliableMulticast Transport
        directive = new SvcConfDirective(true);

        directive.setServiceName("OPENDDS_DCPS_ReliableMulticastLoader");
        directive.setBaseObjectType("Service_Object *");
        directive.setLibrary("ReliableMulticast");
        directive.setFactoryFunction("_make_OPENDDS_DCPS_ReliableMulticastLoader()");

        transports.put("ReliableMulticast", directive);
    }

    private List<SvcConfDirective> directives =
        new ArrayList<SvcConfDirective>();

    public TransportTypeHelper(String transportType) {
        StringTokenizer stok = new StringTokenizer(transportType, Configuration.DELIMITERS);
        while (stok.hasMoreTokens()) {
            String key = stok.nextToken();

            SvcConfDirective directive = transports.get(key);
            if (directive == null) {
                throw new IllegalArgumentException("Unknown transport type: " + key);
            }

            directives.add(directive);
        }
    }

    public List<SvcConfDirective> getSvcConfDirectives() {
        return Collections.unmodifiableList(directives);
    }
}
