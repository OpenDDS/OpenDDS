package org.omg.CORBA;

public final class INITIALIZE extends SystemException {

  public INITIALIZE() {
    this(null);
  }

  public INITIALIZE(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public INITIALIZE(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public INITIALIZE(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
