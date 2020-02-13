package org.omg.CORBA;

public final class ACTIVITY_REQUIRED extends SystemException {

  public ACTIVITY_REQUIRED() {
    this(null);
  }

  public ACTIVITY_REQUIRED(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public ACTIVITY_REQUIRED(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public ACTIVITY_REQUIRED(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
