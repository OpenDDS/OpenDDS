/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS.DCPS;

public final class TheServiceParticipant {

    private TheServiceParticipant() {}

    public static native void shutdown();

    public static native int domain_to_repo(int domain);

    public static native void set_repo_domain(int domain, int repo);

    public static native void set_repo_ior(String ior, int repo);
}
