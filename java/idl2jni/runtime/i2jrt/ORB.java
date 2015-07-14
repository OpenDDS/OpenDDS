/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package i2jrt;
import org.omg.CORBA.StringSeqHolder;

public class ORB {

  //JNI implementation details (C++ peer object)
  private long _jni_ptr;

  protected ORB(long ptr) {
    _jni_ptr = ptr;
  }

  private native void _jni_fini();

  protected void finalize() {
    _jni_fini();
  }

  public static native ORB init(StringSeqHolder args, String id);

  public static ORB init(StringSeqHolder args) {
    return ORB.init(args, "");
  }

  public native String object_to_string(org.omg.CORBA.Object obj);

  public native org.omg.CORBA.Object string_to_object(String str);

  public native void shutdown(boolean wait_for_completion)
    throws org.omg.CORBA.BAD_INV_ORDER;

  public native void destroy()
    throws org.omg.CORBA.BAD_INV_ORDER;

  static {
    String libName = "idl2jni_runtime";
    String propVal = System.getProperty("opendds.native.debug");
    if (propVal != null && ("1".equalsIgnoreCase(propVal) ||
        "y".equalsIgnoreCase(propVal) || "yes".equalsIgnoreCase(propVal)
        || "t".equalsIgnoreCase(propVal) ||
        "true".equalsIgnoreCase(propVal))) libName = libName + "d";
    System.loadLibrary(libName);
  }

}
