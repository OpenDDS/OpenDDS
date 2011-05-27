package OpenDDS.DCPS.transport;

public class AttachStatus {

  public static final int _ATTACH_BAD_TRANSPORT = 0;
  public static final AttachStatus ATTACH_BAD_TRANSPORT = new AttachStatus(_ATTACH_BAD_TRANSPORT);

  public static final int _ATTACH_ERROR = 1;
  public static final AttachStatus ATTACH_ERROR = new AttachStatus(_ATTACH_ERROR);

  public static final int _ATTACH_INCOMPATIBLE_QOS = 2;
  public static final AttachStatus ATTACH_INCOMPATIBLE_QOS = new AttachStatus(_ATTACH_INCOMPATIBLE_QOS);

  public static final int _ATTACH_OK = 3;
  public static final AttachStatus ATTACH_OK = new AttachStatus(_ATTACH_OK);

  public int value() { return _value; }
  private int _value;
  public static AttachStatus from_int(int value) {
    return new AttachStatus (value);
  }
  protected AttachStatus(int value) { _value = value; }
}
