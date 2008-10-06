package OpenDDS.DCPS.transport;

public final class TransportImpl {

    private TransportImpl(long ptr) {
        _jni_pointer = ptr;
    }

    private native void _jni_fini();

    private long _jni_pointer;

    protected void finalize() {
        _jni_fini();
    }

    public native AttachStatus attach_to_publisher(DDS.Publisher pub);

    public native AttachStatus attach_to_subscriber(DDS.Subscriber sub);

}
