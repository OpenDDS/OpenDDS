package org.omg.CORBA;

public final class TRANSACTION_ROLLEDBACK extends SystemException {

  public TRANSACTION_ROLLEDBACK() {
    this(null);
  }

  public TRANSACTION_ROLLEDBACK(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public TRANSACTION_ROLLEDBACK(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public TRANSACTION_ROLLEDBACK(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
