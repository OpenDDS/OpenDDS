/*
 * $Id$
 */

package i2jrt;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class Runtime {

    static {
        String library = "idl2jni_runtime";
        if (Boolean.getBoolean("opendds.native.debug")) {
            library = library.concat("d");
        }
        System.loadLibrary(library);
    }

    public static native void setClassLoader(ClassLoader cl);

    //

    private Runtime() {}
}
