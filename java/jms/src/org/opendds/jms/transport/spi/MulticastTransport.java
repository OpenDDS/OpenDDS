/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.transport.spi;

import org.opendds.jms.management.argument.SvcConfDirective;

/**
 * @author  Steven Stallion
 */
public class MulticastTransport implements Transport {

    public String getName() {
        return "multicast";
    }

    public SvcConfDirective getDirective() {
        SvcConfDirective directive = new SvcConfDirective();

        directive.setDynamic(true);
        directive.setServiceName("OpenDDS_DCPS_Multicast_Service");
        directive.setBaseObjectType("Service_Object *");
        directive.setLibrary("OpenDDS_Multicast");
        directive.setFactoryFunction("_make_MulticastLoader()");

        return directive;
    }
}
