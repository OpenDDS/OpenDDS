package org.omg.CORBA;

public final class COMM_FAILURE extends SystemException {

  public COMM_FAILURE() {
    this(null);
  }

  public COMM_FAILURE(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public COMM_FAILURE(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public COMM_FAILURE(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
