package org.omg.CORBA;

public final class TRANSACTION_UNAVAILABLE extends SystemException {

  public TRANSACTION_UNAVAILABLE() {
    this(null);
  }

  public TRANSACTION_UNAVAILABLE(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public TRANSACTION_UNAVAILABLE(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public TRANSACTION_UNAVAILABLE(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
