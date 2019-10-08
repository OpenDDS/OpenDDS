package org.omg.CORBA;

public final class INVALID_ACTIVITY extends SystemException {

  public INVALID_ACTIVITY() {
    this(null);
  }

  public INVALID_ACTIVITY(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public INVALID_ACTIVITY(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public INVALID_ACTIVITY(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
