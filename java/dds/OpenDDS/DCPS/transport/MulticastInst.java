/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS.DCPS.transport;

public class MulticastInst extends TransportInst {

    MulticastInst(long ptr) {
        super(ptr);
    }

    public String getType() { return "multicast"; }

    public native boolean getDefaultToIPv6();
    public native void setDefaultToIPv6(boolean dtip6);

    public native short getPortOffset();
    public native void setPortOffset(short po);

    public native String getGroupAddress();
    public native void setGroupAddress(String ga);

    public native boolean getReliable();
    public native void setReliable(boolean r);

    public native double getSynBackoff();
    public native void setSynBackoff(double sb);

    public native long getSynInterval();
    public native void setSynInterval(long si);

    public native long getSynTimeout();
    public native void setSynTimeout(long st);

    public native int getNakDepth();
    public native void setNakDepth(int nd);

    public native long getNakInterval();
    public native void setNakInterval(long ni);

    public native int getNakDelayInterval();
    public native void setNakDelayInterval(int ndi);

    public native int getNakMax();
    public native void setNakMax(int nm);

    public native long getNakTimeout();
    public native void setNakTimeout(long nt);

    public native byte getTimeToLive();
    public native void setTimeToLive(byte ttl);

    public native int getRcvBufferSize();
    public native void setRcvBufferSize(int rbs);
}
