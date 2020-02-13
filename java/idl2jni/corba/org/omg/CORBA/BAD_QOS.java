package org.omg.CORBA;

public final class BAD_QOS extends SystemException {

  public BAD_QOS() {
    this(null);
  }

  public BAD_QOS(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public BAD_QOS(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public BAD_QOS(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
