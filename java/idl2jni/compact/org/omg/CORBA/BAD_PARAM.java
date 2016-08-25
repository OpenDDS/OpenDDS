package org.omg.CORBA;

public final class BAD_PARAM extends SystemException {

  public BAD_PARAM() {
    this("");
  }

  public BAD_PARAM(String s) {
    this(s, 0, CompletionStatus.COMPLETED_NO);
  }

  public BAD_PARAM(int i, CompletionStatus c) {
    this("", i, c);
  }

  public BAD_PARAM(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
