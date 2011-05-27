/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.transport.spi;

import org.opendds.jms.management.argument.SvcConfDirective;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class SimpleUdpTransport implements Transport {

    public String getName() {
        return "SimpleUdp";
    }

    public SvcConfDirective getDirective() {
        SvcConfDirective directive = new SvcConfDirective();

        directive.setDynamic(true);
        directive.setServiceName("OPENDDS_DCPS_SimpleUnreliableDgramLoader");
        directive.setBaseObjectType("Service_Object *");
        directive.setLibrary("SimpleUnreliableDgram");
        directive.setFactoryFunction("_make_OPENDDS_DCPS_SimpleUnreliableDgramLoader()");
        directive.addOption("-type");
        directive.addOption("SimpleUdp");

        return directive;
    }
}
