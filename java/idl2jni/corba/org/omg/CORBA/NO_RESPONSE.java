package org.omg.CORBA;

public final class NO_RESPONSE extends SystemException {

  public NO_RESPONSE() {
    this(null);
  }

  public NO_RESPONSE(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public NO_RESPONSE(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public NO_RESPONSE(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
