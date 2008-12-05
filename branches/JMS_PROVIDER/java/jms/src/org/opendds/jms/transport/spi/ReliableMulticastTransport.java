/*
 * $Id$
 */

package org.opendds.jms.transport.spi;

import org.opendds.jms.management.argument.SvcConfDirective;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ReliableMulticastTransport implements Transport {

    public String getName() {
        return "ReliableMulticast";
    }

    public SvcConfDirective getDirective() {
        SvcConfDirective directive = new SvcConfDirective();

        directive.setDynamic(true);
        directive.setServiceName("OPENDDS_DCPS_ReliableMulticastLoader");
        directive.setBaseObjectType("Service_Object *");
        directive.setLibrary("ReliableMulticast");
        directive.setFactoryFunction("_make_OPENDDS_DCPS_ReliableMulticastLoader()");

        return directive;
    }
}
