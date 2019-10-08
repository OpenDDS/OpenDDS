package org.omg.CORBA;

public final class OBJECT_NOT_EXIST extends SystemException {

  public OBJECT_NOT_EXIST() {
    this(null);
  }

  public OBJECT_NOT_EXIST(String s) {
    this(s, 0, CompletionStatus.COMPLETED_MAYBE);
  }

  public OBJECT_NOT_EXIST(int i, CompletionStatus c) {
    this(null, i, c);
  }

  public OBJECT_NOT_EXIST(String s, int i, CompletionStatus c) {
    super(s, i, c);
  }

}
