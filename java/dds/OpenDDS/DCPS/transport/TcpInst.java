/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS.DCPS.transport;

public class TcpInst extends TransportInst {

    TcpInst(long ptr) {
        super(ptr);
    }

    public String getType() { return "Tcp"; }

    public native String getLocalAddress();
    public native void setLocalAddress(String la);

    public native boolean isEnableNagleAlgorithm();
    public native void setEnableNagleAlgorithm(boolean ena);

    public native int getConnRetryInitialDelay();
    public native void setConnRetryInitialDelay(int crid);

    public native double getConnRetryBackoffMultiplier();
    public native void setConnRetryBackoffMultiplier(double crbm);

    public native int getConnRetryAttempts();
    public native void setConnRetryAttempts(int cra);

    public native int getMaxOutputPausePeriod();
    public native void setMaxOutputPausePeriod(int mopp);

    public native int getPassiveReconnectDuration();
    public native void setPassiveReconnectDuration(int prd);
}
