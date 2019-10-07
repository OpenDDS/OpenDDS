package org.omg.CORBA;

public final class TRANSACTION_REQUIRED extends SystemException {

  public TRANSACTION_REQUIRED() {
    this(null);
  }

  public TRANSACTION_REQUIRED(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public TRANSACTION_REQUIRED(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public TRANSACTION_REQUIRED(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
