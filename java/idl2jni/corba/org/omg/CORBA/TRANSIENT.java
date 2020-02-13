package org.omg.CORBA;

public final class TRANSIENT extends SystemException {

  public TRANSIENT() {
    this(null);
  }

  public TRANSIENT(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public TRANSIENT(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public TRANSIENT(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
