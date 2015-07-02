/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

/**
 * Class responsible for managing an in-process DCPSInfoRepo
 * instance.
 * <p/>
 * The general contract of this is object is such that only a single
 * instance may exist at any given time.  Care must be taken to
 * ensure an existing DCPSInfoRepo instance is shutdown prior to
 * creating additional instances.
 *
 * @author  Steven Stallion
 */
public final class DCPSInfoRepo implements Runnable {

    static {
        String library = "opendds-jms-native";
        if (Boolean.getBoolean("opendds.native.debug")) {
            library = library.concat("d");
        }
        System.loadLibrary(library);
    }

    private long peer;

    /**
     * Constructs and initializes a DCPSInfoRepo instance.
     *
     * @throws  IllegalArgumentException if an invalid argument
     *          causes DCPSInfoRepo initialization to fail
     * @throws  NullPointerException if {@code args} is {@code null}
     *          or contains a {@code null} value
     * @throws  org.omg.CORBA.UNKNOWN if the DCPSInfoRepo fails
     *          internally with a CORBA exception
     */
    public DCPSInfoRepo(String[] args) {
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
     *          has been shutdown
     */
    public native void run();

    /**
     * Gracefully terminates a DCPSInfoRepo instance running on
     * another thread.
     *
     * @throws  IllegalStateException if the DCPSInfoRepo instance
     *          has been shutdown
     */
    public native void shutdown();

    @Override
    protected void finalize() throws Throwable {
        fini(); // always finalize peer first
        super.finalize();
    }
}
