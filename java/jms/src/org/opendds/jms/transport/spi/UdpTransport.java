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
public class UdpTransport implements Transport {

    public String getName() {
        return "udp";
    }

    public SvcConfDirective getDirective() {
        SvcConfDirective directive = new SvcConfDirective();

        directive.setDynamic(true);
        directive.setServiceName("OpenDDS_DCPS_Udp_Service");
        directive.setBaseObjectType("Service_Object *");
        directive.setLibrary("OpenDDS_Udp");
        directive.setFactoryFunction("_make_UdpLoader()");

        return directive;
    }
}
