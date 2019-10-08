package org.omg.CORBA;

public final class INVALID_TRANSACTION extends SystemException {

  public INVALID_TRANSACTION() {
    this(null);
  }

  public INVALID_TRANSACTION(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public INVALID_TRANSACTION(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public INVALID_TRANSACTION(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
