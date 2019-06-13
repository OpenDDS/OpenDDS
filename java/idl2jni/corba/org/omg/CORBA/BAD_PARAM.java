package org.omg.CORBA;

public final class BAD_PARAM extends SystemException {

  public BAD_PARAM() {
    this(null);
  }

  public BAD_PARAM(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public BAD_PARAM(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public BAD_PARAM(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
