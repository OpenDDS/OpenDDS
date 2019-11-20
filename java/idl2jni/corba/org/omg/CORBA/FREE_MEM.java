package org.omg.CORBA;

public final class FREE_MEM extends SystemException {

  public FREE_MEM() {
    this(null);
  }

  public FREE_MEM(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public FREE_MEM(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public FREE_MEM(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
