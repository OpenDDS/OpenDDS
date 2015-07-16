/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS.DCPS;

public final class TheParticipantFactory {

  private TheParticipantFactory() {}

  public static native DDS.DomainParticipantFactory
    WithArgs(org.omg.CORBA.StringSeqHolder args);

  public static native DDS.DomainParticipantFactory
    getInstance();

  static {
    loadNativeLib();
  }

  // Even though this is public, the user shouldn't need to call it.  The first
  // class loaded will have a static initializer which calls this function.
  // It is only made public so that other OpenDDS packages can use it.
  public static void loadNativeLib () {
    String propVal = System.getProperty("opendds.native.debug");
    if (propVal != null && ("1".equalsIgnoreCase(propVal) ||
        "y".equalsIgnoreCase(propVal) ||
        "yes".equalsIgnoreCase(propVal) ||
        "t".equalsIgnoreCase(propVal) ||
        "true".equalsIgnoreCase(propVal)))
      System.loadLibrary("OpenDDS_DCPS_Javad");
    else System.loadLibrary("OpenDDS_DCPS_Java");
  }
}
