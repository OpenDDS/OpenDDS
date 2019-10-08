package org.omg.CORBA;

public final class UNKNOWN extends SystemException {

  public UNKNOWN() {
    this(null);
  }

  public UNKNOWN(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public UNKNOWN(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public UNKNOWN(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
