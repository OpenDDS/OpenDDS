package org.omg.CORBA;

public final class OBJ_ADAPTER extends SystemException {

  public OBJ_ADAPTER() {
    this(null);
  }

  public OBJ_ADAPTER(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public OBJ_ADAPTER(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public OBJ_ADAPTER(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
