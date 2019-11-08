package org.omg.CORBA;

public final class NO_MEMORY extends SystemException {

  public NO_MEMORY() {
    this(null);
  }

  public NO_MEMORY(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public NO_MEMORY(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public NO_MEMORY(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
