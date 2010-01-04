/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
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
public class SimpleTcpTransport implements Transport {

    public String getName() {
        return "SimpleTcp";
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
