package org.omg.CORBA;

public final class TIMEOUT extends SystemException {

  public TIMEOUT() {
    this(null);
  }

  public TIMEOUT(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public TIMEOUT(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public TIMEOUT(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
