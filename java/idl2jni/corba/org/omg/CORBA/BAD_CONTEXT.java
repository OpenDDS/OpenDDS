package org.omg.CORBA;

public final class BAD_CONTEXT extends SystemException {

  public BAD_CONTEXT() {
    this(null);
  }

  public BAD_CONTEXT(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public BAD_CONTEXT(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public BAD_CONTEXT(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
