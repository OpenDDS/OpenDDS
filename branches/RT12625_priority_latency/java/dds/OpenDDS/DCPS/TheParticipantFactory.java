package OpenDDS.DCPS;

public final class TheParticipantFactory {

  private TheParticipantFactory() {}

  public static native DDS.DomainParticipantFactory
    WithArgs(org.omg.CORBA.StringSeqHolder args);

  static {
    String propVal = System.getProperty("jni.nativeDebug");
    if (propVal != null && ("1".equalsIgnoreCase(propVal) ||
        "y".equalsIgnoreCase(propVal) ||
        "yes".equalsIgnoreCase(propVal) ||
        "t".equalsIgnoreCase(propVal) ||
        "true".equalsIgnoreCase(propVal)))
      System.loadLibrary("OpenDDS_DCPS_Javad");
    else System.loadLibrary("OpenDDS_DCPS_Java");
  }

}
