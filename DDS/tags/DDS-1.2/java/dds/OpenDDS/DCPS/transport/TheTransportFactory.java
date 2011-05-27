package OpenDDS.DCPS.transport;

public final class TheTransportFactory {

    private TheTransportFactory() {}

    public static final boolean AUTO_CONFIG = true;

    public static native TransportImpl create_transport_impl(int id, boolean auto_configure);

    public static native void release();

}
