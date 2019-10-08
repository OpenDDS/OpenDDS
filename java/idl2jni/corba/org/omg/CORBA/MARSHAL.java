package org.omg.CORBA;

public final class MARSHAL extends SystemException {

  public MARSHAL() {
    this(null);
  }

  public MARSHAL(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public MARSHAL(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public MARSHAL(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
