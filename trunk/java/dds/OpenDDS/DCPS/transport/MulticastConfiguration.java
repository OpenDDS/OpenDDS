/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS.DCPS.transport;

public class MulticastConfiguration extends TransportConfiguration {

    MulticastConfiguration(int id) {
        super(id);
    }

    native void saveSpecificConfig(long cfg);
    native void loadSpecificConfig(long cfg);

    public String getType() { return "multicast"; }
    
    private boolean defaultToIPv6;
    public boolean getDefaultToIPv6() { return defaultToIPv6; }
    public void setDefaultToIPv6(boolean dtip6) { defaultToIPv6 = dtip6; }
    
    private short portOffset;
    public short getPortOffset() { return portOffset; }
    public void setPortOffset(short po) { portOffset = po; }
    
    private String groupAddress;
    public String getGroupAddress() { return groupAddress; }
    public void setGroupAddress(String ga) { groupAddress = ga; }

    private boolean reliable;
    public boolean getReliable() { return reliable; }
    public void setReliable(boolean r) { reliable = r; }

    private long synInterval;
    public long getSynInterval() { return synInterval; }
    public void setSynInterval(long si) { synInterval = si; }

    private long synTimeout;
    public long getSynTimeout() { return synTimeout; }
    public void setSynTimeout(long st) { synTimeout = st; }

    private long nakInterval;
    public long getNakInterval() { return nakInterval; }
    public void setNakInterval(long ni) { nakInterval = ni; }

    private long nakTimeout;
    public long getNakTimeout() { return nakTimeout; }
    public void setNakTimeout(long nt) { nakTimeout = nt; }

    private int nakRepairSize;
    public int getNakRepairSize() { return nakRepairSize; }
    public void setNakRepairSize(int nrs) { nakRepairSize = nrs; }
}
