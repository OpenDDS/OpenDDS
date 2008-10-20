/*
 * $Id$
 */

package org.opendds.jms.ir;

/**
 * Class responsible for managing an in-process DCPSInfoRepo
 * instance.
 * <p/>
 * The general contract of this is object is such that only a single
 * instance should exist at any given time.  Those who wish to create
 * and destroy multiple DCPSInfoRepo instances within the same
 * process should ensure the {@code shutdown()} method is always
 * called with {@code finalize} set as {@code true}.
 *
 * @author  Steven Stallion
 * @version $Revision$
 */
public final class DCPSInfoRepoService implements Runnable {
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
     */
    public native void run();

    /**
     * Gracefully terminates a DCPSInfoRepo instance running on
     * another thread.  By default, the {@code shutdown()} method
     * defers DCPSInfoRepo finalization until the
     * {@code DCPSInfoRepoService} is marked for collection.
     *
     * @throws  IllegalStateException if the DCPSInfoRepo instance
     *          has been finalized and marked for collection
     */
    public void shutdown() {
        shutdown(false);
    }

    /**
     * Gracefully terminates a DCPSInfoRepo instance running on
     * another thread.
     *
     * @param   finalize indicates the DCPSInfoRepo instance should
     *          be finalized after shuting down
     *
     * @throws  IllegalStateException if the DCPSInfoRepo instance
     *          has been finalized and marked for collection
     */
    public native void shutdown(boolean finalize);

    @Override
    protected void finalize() throws Throwable {
        fini(); // always finalize peer first
        super.finalize();
    }
}
