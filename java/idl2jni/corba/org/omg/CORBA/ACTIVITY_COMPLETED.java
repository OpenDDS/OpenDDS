package org.omg.CORBA;

public final class ACTIVITY_COMPLETED extends SystemException {

  public ACTIVITY_COMPLETED() {
    this(null);
  }

  public ACTIVITY_COMPLETED(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public ACTIVITY_COMPLETED(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public ACTIVITY_COMPLETED(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
