/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS.DCPS.transport;

public class UdpConfiguration extends TransportConfiguration {

    UdpConfiguration(int id) {
        super(id);
    }

    native void saveSpecificConfig(long cfg);
    native void loadSpecificConfig(long cfg);

    public String getType() { return "udp"; }

    private String localAddress;
    public String getLocalAddress() { return localAddress; }
    public void setLocalAddress(String la) { localAddress = la; }
}
