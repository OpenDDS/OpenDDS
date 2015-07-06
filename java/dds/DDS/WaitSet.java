/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package DDS;

public class WaitSet extends _WaitSetInterfTAOPeer {

    public WaitSet() {
        super(_jni_init());
    }

    private static native long _jni_init();
}
