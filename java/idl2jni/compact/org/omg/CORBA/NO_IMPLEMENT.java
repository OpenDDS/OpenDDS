package org.omg.CORBA;

public final class NO_IMPLEMENT extends SystemException {

  public NO_IMPLEMENT() {
    this("");
  }

  public NO_IMPLEMENT(String s) {
    this(s, 0, CompletionStatus.COMPLETED_NO);
  }

  public NO_IMPLEMENT(int i, CompletionStatus c) {
    this("", i, c);
  }

  public NO_IMPLEMENT(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
