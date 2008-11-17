/*
 * $Id$
 */

package org.opendds.jms.transport.spi;

import OpenDDS.DCPS.transport.SimpleTcpConfiguration;

import org.opendds.jms.common.SvcConfDirective;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class SimpleTcpTransport implements Transport {

    public String getName() {
        return "SimpleTcp";
    }

    public Class getConfigurationClass() {
        return SimpleTcpConfiguration.class;
    }
    
    public SvcConfDirective getDirective() {
        SvcConfDirective directive = new SvcConfDirective();

        directive.setDynamic(true);
        directive.setServiceName("DCPS_SimpleTcpLoader");
        directive.setBaseObjectType("Service_Object *");
        directive.setLibrary("SimpleTcp");
        directive.setFactoryFunction("_make_DCPS_SimpleTcpLoader()");
        directive.addOption("-type");
        directive.addOption("SimpleTcp");

        return directive;
    }
}
