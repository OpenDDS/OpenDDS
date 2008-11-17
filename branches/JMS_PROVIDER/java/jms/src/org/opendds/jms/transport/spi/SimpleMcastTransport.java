/*
 * $Id$
 */

package org.opendds.jms.transport.spi;

import OpenDDS.DCPS.transport.SimpleMcastConfiguration;

import org.opendds.jms.common.SvcConfDirective;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class SimpleMcastTransport implements Transport {

    public String getName() {
        return "SimpleMcast";
    }

    public Class getConfigurationClass() {
        return SimpleMcastConfiguration.class;
    }

    public SvcConfDirective getDirective() {
        SvcConfDirective directive = new SvcConfDirective();

        directive.setDynamic(true);
        directive.setServiceName("OPENDDS_DCPS_SimpleUnreliableDgramLoader");
        directive.setBaseObjectType("Service_Object *");
        directive.setLibrary("SimpleUnreliableDgram");
        directive.setFactoryFunction("_make_OPENDDS_DCPS_SimpleUnreliableDgramLoader()");
        directive.addOption("-type");
        directive.addOption("SimpleMcast");

        return directive;
    }
}
