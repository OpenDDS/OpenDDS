/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS.DCPS.transport;

public class SimpleUdpConfiguration
    extends SimpleUnreliableDgramConfiguration {

    SimpleUdpConfiguration(int id) {
        super(id);
    }

    public String getType() { return "SimpleUdp"; }
}
