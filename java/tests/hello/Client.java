/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

public class Client {

  public static void main(String[] args) {
    org.omg.CORBA.StringSeqHolder args_holder =
      new org.omg.CORBA.StringSeqHolder(args);
    i2jrt.ORB orb = i2jrt.ORB.init(args_holder);

    if (orb == null) {
      System.err.println("ERROR: Failed to init ORB");
      System.exit(-1);
    }

    if (args_holder.value.length < 2 || !"-k".equals(args_holder.value[0])) {
      System.err.println("usage: Client -k <ior>");
      System.exit(-1);
    }

    String ior = args_holder.value[1];
    org.omg.CORBA.Object obj = orb.string_to_object(ior);
    Test.Hello hello = Test.HelloHelper.narrow(obj);
    if (hello == null) {
      System.err.println("ERROR: couldn't get the Hello object reference");
      System.exit(-1);
    }

    String the_string = hello.get_string();
    System.out.println ("Hello Client - get_string returned <" + the_string
      + ">");
    hello.shutdown();
    orb.destroy();
  }
}
