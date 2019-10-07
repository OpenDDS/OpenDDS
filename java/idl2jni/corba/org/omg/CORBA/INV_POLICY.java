package org.omg.CORBA;

public final class INV_POLICY extends SystemException {

  public INV_POLICY() {
    this(null);
  }

  public INV_POLICY(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public INV_POLICY(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public INV_POLICY(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
