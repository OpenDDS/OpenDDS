package org.omg.CORBA;

public final class INV_FLAG extends SystemException {

  public INV_FLAG() {
    this(null);
  }

  public INV_FLAG(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public INV_FLAG(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public INV_FLAG(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
