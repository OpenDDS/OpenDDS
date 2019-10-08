package org.omg.CORBA;

public final class INTERNAL extends SystemException {

  public INTERNAL() {
    this(null);
  }

  public INTERNAL(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public INTERNAL(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public INTERNAL(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
