/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS.DCPS.transport;

public class RtpsUdpInst extends TransportInst {

    RtpsUdpInst(long ptr) {
        super(ptr);
    }

    public String getType() { return "rtps_udp"; }

    public native String getLocalAddress();
    public native void setLocalAddress(String la);

    public native boolean isUseMulticast();
    public native void setUseMulticast(boolean um);

    public native String getMulticastGroupAddress();
    public native void setMulticastGroupAddress(String mga);

    public native int getNakDepth();
    public native void setNakDepth(int nd);

    public native long getNakResponseDelay();
    public native void setNakResponseDelay(long nrd);

    public native long getHeartbeatPeriod();
    public native void setHeartbeatPeriod(long hbp);

    public native long getHeartbeatResponseDelay();
    public native void setHeartbeatResponseDelay(long hrd);

    public native long getHandshakeTimeout();
    public native void setHandshakeTimeout(long ht);
}
