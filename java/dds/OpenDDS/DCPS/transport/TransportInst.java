/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS.DCPS.transport;

import java.io.Serializable;

public abstract class TransportInst implements Serializable {

    private long _jni_pointer;

    TransportInst(long ptr) {
        _jni_pointer = ptr;
    }

    private native void _jni_fini();

    protected void finalize() {
        _jni_fini();
    }

    public abstract String getType();

    public native String getName();

    public native int getQueueMessagesPerPool();
    public native void setQueueMessagesPerPool(int qmpp);

    public native int getQueueInitialPools();
    public native void setQueueInitialPools(int qip);

    public native int getMaxPacketSize();
    public native void setMaxPacketSize(int mps);

    public native int getMaxSamplesPerPacket();
    public native void setMaxSamplesPerPacket(int mspp);

    public native int getOptimumPacketSize();
    public native void setOptimumPacketSize(int ops);

    public native boolean isThreadPerConnection();
    public native void setThreadPerConnection(boolean tpc);

    public native int getDatalinkReleaseDelay();
    public native void setDatalinkReleaseDelay(int drd);

    public native int getDatalinkControlChunks();
    public native void setDatalinkControlChunks(int dcc);
}
