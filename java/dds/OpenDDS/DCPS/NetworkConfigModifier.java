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

    public native void add_interface(String name);
    public native void remove_interface(String name);
    public native void add_address(String name, String address);
    public native void remove_address(String name, String address);
}