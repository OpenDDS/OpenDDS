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
public class TcpTransport implements Transport {

    public String getName() {
        return "tcp";
    }

    public SvcConfDirective getDirective() {
        SvcConfDirective directive = new SvcConfDirective();

        directive.setDynamic(true);
        directive.setServiceName("OpenDDS_Tcp");
        directive.setBaseObjectType("Service_Object *");
        directive.setLibrary("OpenDDS_Tcp");
        directive.setFactoryFunction("_make_TcpLoader()");

        return directive;
    }
}
