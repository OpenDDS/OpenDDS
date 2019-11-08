/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package i2jrt;
import org.omg.CORBA.*;

public class TAOObject implements org.omg.CORBA.Object {

  //JNI implementation details (C++ peer object)
  private long _jni_ptr;

  protected TAOObject(long ptr) {
    _jni_ptr = ptr;
  }

  private native void _jni_fini();

  protected void finalize() {
    _jni_fini();
  }

  @Override
  public boolean equals(java.lang.Object o) {
    if (o instanceof TAOObject) {
      return _jni_ptr == ((TAOObject)o)._jni_ptr;
    }
    return false;
  }

  @Override
  public int hashCode() {
    return Long.valueOf(_jni_ptr).hashCode();
  }

  //org.omg.CORBA.Object methods
  public native boolean _is_a(String repositoryIdentifier);
  public native boolean _is_equivalent(org.omg.CORBA.Object other);
  public native boolean _non_existent();
  public native int _hash(int maximum);
  public native org.omg.CORBA.Object _duplicate();
  public native void _release();

  //  DII is not implemented, these methods throw NO_IMPLEMENT
  public org.omg.CORBA.Object _get_interface_def() {
    throw new NO_IMPLEMENT();
  }

  public Request _request(String operation) {
    throw new NO_IMPLEMENT();
  }

  public Request _create_request(Context ctx, String operation,
      NVList arg_list, NamedValue result) {
    throw new NO_IMPLEMENT();
  }

  public Request _create_request(Context ctx, String operation,
      NVList arg_list, NamedValue result, ExceptionList exclist,
      ContextList ctxlist) {
    throw new NO_IMPLEMENT();
  }

  //  These methods also throw NO_IMPLEMENT

  public Policy _get_policy(int policy_type) {
    throw new NO_IMPLEMENT();
  }

  public DomainManager[] _get_domain_managers() {
    throw new NO_IMPLEMENT();
  }

  public org.omg.CORBA.Object _set_policy_override(Policy[] policies,
      SetOverrideType set_add) {
    throw new NO_IMPLEMENT();
  }

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
