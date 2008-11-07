package OpenDDS.DCPS;

public final class TheServiceParticipant {

    private TheServiceParticipant() {}

    public static native void shutdown();

    public static native int domain_to_repo(int domain);

    public static native void set_repo_domain(int domain, int repo);

    public static native void set_repo_ior(String ior, int repo);
}
