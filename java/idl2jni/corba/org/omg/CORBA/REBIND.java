package org.omg.CORBA;

public final class REBIND extends SystemException {

  public REBIND() {
    this(null);
  }

  public REBIND(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public REBIND(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public REBIND(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
