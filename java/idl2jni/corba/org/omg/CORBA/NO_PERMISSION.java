package org.omg.CORBA;

public final class NO_PERMISSION extends SystemException {

  public NO_PERMISSION() {
    this(null);
  }

  public NO_PERMISSION(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public NO_PERMISSION(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public NO_PERMISSION(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
