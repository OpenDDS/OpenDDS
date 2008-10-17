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
        System.loadLibrary("opendds-inforepo-native");
    }

    private long peer;

    /**
     * Constructs and initializes a DCPSInfoRepoService instance.
     * Care must be taken to ensure that the {@code args}
     *
     * @throws  IllegalArgumentException    if an invalid argument
     *          causes DCPSInfoRepo initialization to fail
     * @throws  NullPointerException        if {@code args} is
     *          {@code null} or contains a {@code null} value
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
     * @throws  IllegalStateException   if the DCPSInfoRepo instance
     *          has been finalized and marked for garbage collection
     *
     * @see #shutdown
     */
    public native void run();

    /**
     * Gracefully terminates a DCPSInfoRepo instance running on
     * another thread.
     *
     * @throws  IllegalStateException   if the DCPSInfoRepo instance
     *          has been finalized and marked for garbage collection
     */
    public native void shutdown();

    @Override
    protected void finalize() throws Throwable {
        fini(); // always finalize peer first
        super.finalize();
    }
}
