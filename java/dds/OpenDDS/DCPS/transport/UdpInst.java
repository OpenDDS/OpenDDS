/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS.DCPS.transport;

public class UdpInst extends TransportInst {

    UdpInst(long ptr) {
        super(ptr);
    }

    public String getType() { return "udp"; }

    public native String getLocalAddress();
    public native void setLocalAddress(String la);
}
