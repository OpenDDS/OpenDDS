package org.omg.CORBA;

public final class NO_RESOURCES extends SystemException {

  public NO_RESOURCES() {
    this(null);
  }

  public NO_RESOURCES(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public NO_RESOURCES(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public NO_RESOURCES(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
