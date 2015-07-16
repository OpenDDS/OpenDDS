/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS.DCPS.transport;

public final class TheTransportRegistry {

    private TheTransportRegistry() {}

    // Transport type string constants that can be passed as the 2nd parameter
    // to the create_transport_impl() method and to the
    // get_or_create_configuration() method.
    public static final String TRANSPORT_TCP = "tcp";
    public static final String TRANSPORT_UDP = "udp";
    public static final String TRANSPORT_MULTICAST = "multicast";

    public static native TransportInst create_inst(String name,
                                                   String transport_type);
    public static native TransportInst get_inst(String name);
    public static native void remove_inst(TransportInst inst);

    public static native TransportConfig create_config(String name);
    public static native TransportConfig get_config(String name);
    public static native void remove_config(TransportConfig config);

    public static native TransportConfig global_config();
    public static native void global_config(TransportConfig cfg);

    public static native void bind_config(String name, DDS.Entity entity);
    public static native void bind_config(TransportConfig cfg,
                                          DDS.Entity entity);

    /// Release all transports and configurations
    public static native void release();

    static {
        OpenDDS.DCPS.TheParticipantFactory.loadNativeLib();
    }
}
