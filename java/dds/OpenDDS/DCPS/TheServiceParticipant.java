/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS.DCPS;

import DDS.DomainParticipant;

public final class TheServiceParticipant {

    private TheServiceParticipant() {}

    public static native void shutdown();

    public static native String domain_to_repo(int domain);

    public static native void set_repo_domain(int domain, String repo);

    public static native void set_repo_ior(String ior, String repo);

    public static native String get_unique_id(DomainParticipant participant);
}
