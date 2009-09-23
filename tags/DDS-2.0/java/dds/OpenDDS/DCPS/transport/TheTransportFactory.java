/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS.DCPS.transport;

public final class TheTransportFactory {

    private TheTransportFactory() {}

    // Auto-configure constants that lend more meaning to the call to
    // create_transport_impl() than a simple "true" or "false".
    public static final boolean AUTO_CONFIG = true;
    public static final boolean DONT_AUTO_CONFIG = false;

    // Transport type string constants that can be passed as the 2nd parameter
    // to the create_transport_impl() method and to the
    // get_or_create_configuration() method.
    public static final String TRANSPORT_TCP = "SimpleTcp";
    public static final String TRANSPORT_RMCAST = "ReliableMulticast";
    public static final String TRANSPORT_UDP_UNI = "SimpleUdp";
    public static final String TRANSPORT_UDP_MULTI = "SimpleMcast";

    public static native TransportImpl create_transport_impl(int id, boolean auto_configure);
    public static native TransportImpl create_transport_impl(int id, String transportType, boolean auto_configure);

    public static native TransportConfiguration get_or_create_configuration(int id, String type);

    /// Release all transports and configurations
    public static native void release();

    /// Release the specific transport
    public static native void release(int id);

    static {
        OpenDDS.DCPS.TheParticipantFactory.loadNativeLib();
    }
}
