public class TestUnion {

  private static native U getUnion(U u);

  public static void main(String[] args) {
    String lib = "idl2jni_test_union";
    String propVal = System.getProperty("opendds.native.debug");
    if (propVal != null && ("1".equalsIgnoreCase(propVal) ||
        "y".equalsIgnoreCase(propVal) ||
        "yes".equalsIgnoreCase(propVal) ||
        "t".equalsIgnoreCase(propVal) ||
        "true".equalsIgnoreCase(propVal)))
      lib += "d";
    System.loadLibrary(lib);

    U[] us = {new U(), new U(), new U()};
    us[0].__default(E.E1);
    us[1].o((byte) 1);
    us[2].s((short) 2);

    for (U u : us) {
      System.out.println("Discriminator: " + getUnion(u).discriminator().value());
    }
  }
}
