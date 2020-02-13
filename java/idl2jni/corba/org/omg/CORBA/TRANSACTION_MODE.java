package org.omg.CORBA;

public final class TRANSACTION_MODE extends SystemException {

  public TRANSACTION_MODE() {
    this(null);
  }

  public TRANSACTION_MODE(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public TRANSACTION_MODE(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public TRANSACTION_MODE(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
