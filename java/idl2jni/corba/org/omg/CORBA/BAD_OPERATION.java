package org.omg.CORBA;

public final class BAD_OPERATION extends SystemException {

  public BAD_OPERATION() {
    this(null);
  }

  public BAD_OPERATION(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public BAD_OPERATION(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public BAD_OPERATION(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
