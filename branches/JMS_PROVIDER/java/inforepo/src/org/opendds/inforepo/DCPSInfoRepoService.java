/*
 * $Id$
 */

package org.opendds.inforepo;

/**
 * Class responsible for managing an in-process DCPSInfoRepo
 * instance.
 *
 * @author  Steven Stallion
 * @version $Revision$
 */
public final class DCPSInfoRepoService implements Runnable {

    static {
        String library = "opendds-inforepo-native";
        if (Boolean.getBoolean("jni.nativeDebug")) {
            library = library.concat("d");  // Win32 hosts only
        }
        System.loadLibrary(library);
    }

    private long peer;

    /**
     * Constructs and initializes a DCPSInfoRepoService instance.
     *
     * @throws  IllegalArgumentException if an invalid argument
     *          causes DCPSInfoRepo initialization to fail
     * @throws  NullPointerException if {@code args} is {@code null}
     *          or contains a {@code null} value
     * @throws  org.omg.CORBA.UNKNOWN if the DCPSInfoRepo fails
     *          internally with a CORBA exception
     */
    public DCPSInfoRepoService(String[] args) {
        init(args);
    }

    protected native void init(String[] args);

    protected native void fini();

    /**
     * Starts a DCPSInfoRepo instance on the current thread.  This
     * method will block until the DCPSInfoRepo is terminated by
     * the {@code shutdown()} method.
     *
     * @throws  IllegalStateException if the DCPSInfoRepo instance
     *          has been finalized and marked for collection
     *
     * @see     #shutdown
     */
    public native void run();

    /**
     * Gracefully terminates a DCPSInfoRepo instance running on
     * another thread.
     *
     * @throws  IllegalStateException if the DCPSInfoRepo instance
     *          has been finalized and marked for collection
     */
    public native void shutdown();

    @Override
    protected void finalize() throws Throwable {
        fini(); // always finalize peer first
        super.finalize();
    }
}
