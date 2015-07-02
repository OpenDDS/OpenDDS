/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS.DCPS.transport;

public class TransportConfig {

    private long _jni_pointer;

    TransportConfig(long ptr) {
        _jni_pointer = ptr;
    }

    private native void _jni_fini();

    protected void finalize() {
        _jni_fini();
    }

    public native String getName();

    public native void addLast(TransportInst inst);

    public native long countInstances();

    public native TransportInst getInstance(long index);

    public native boolean getSwapBytes();
    public native void setSwapBytes(boolean swap);

    public native int getPassiveConnectDuration();
    public native void setPassiveConnectDuration(int pcd);
}
