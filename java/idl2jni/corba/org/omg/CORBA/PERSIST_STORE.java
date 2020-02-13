package org.omg.CORBA;

public final class PERSIST_STORE extends SystemException {

  public PERSIST_STORE() {
    this(null);
  }

  public PERSIST_STORE(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public PERSIST_STORE(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public PERSIST_STORE(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
