public class TestUnion {

  private static native NoDefault getUnionND(NoDefault u);
  private static native WithDefault getUnionWD(WithDefault u);

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

    NoDefault[] nds = {new NoDefault(), new NoDefault(), new NoDefault(), new NoDefault()};
    nds[0].__default(E.E1);
    nds[1].o((byte) 1);
    nds[2].s((short) 2);
    nds[3].s(E.E4, (short) 3);

    boolean failed = false;
    for (NoDefault u1 : nds) {
      final NoDefault u2 = getUnionND(u1);
      if (u1.discriminator() != u2.discriminator()) {
        System.out.println("ERROR: NoDefault expected discriminator " +
                           u1.discriminator() + " != actual " +
                           u2.discriminator());
        failed = true;
      }
    }

    WithDefault[] wds = {new WithDefault(), new WithDefault(), new WithDefault(), new WithDefault()};
    wds[0].b(true);
    wds[1].s(E.E2, "");
    wds[2].s(E.E3, "");
    wds[3].s(E.E4, "");

    for (WithDefault u1 : wds) {
      final WithDefault u2 = getUnionWD(u1);
      if (u1.discriminator() != u2.discriminator()) {
        System.out.println("ERROR: WithDefault expected discriminator " +
                           u1.discriminator() + " != actual " +
                           u2.discriminator());
        failed = true;
      }
    }

    System.exit(failed ? 1 : 0);
  }
}
