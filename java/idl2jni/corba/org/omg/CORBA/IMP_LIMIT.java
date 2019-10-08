package org.omg.CORBA;

public final class IMP_LIMIT extends SystemException {

  public IMP_LIMIT() {
    this(null);
  }

  public IMP_LIMIT(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public IMP_LIMIT(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public IMP_LIMIT(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
