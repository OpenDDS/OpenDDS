/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS.DCPS;


public final class NetworkConfigModifier {

    private long _jni_pointer;

    NetworkConfigModifier(long ptr) {
        _jni_pointer = ptr;
    }

    private native void _jni_fini();

    protected void finalize() {
        _jni_fini();
    }

    public native void update_interfaces();
}
