package OpenDDS.DCPS.transport;

public abstract class SimpleUnreliableDgramConfiguration
    extends TransportConfiguration {

    SimpleUnreliableDgramConfiguration(int id) {
        super(id);
    }

    native void saveSpecificConfig(long cfg);
    native void loadSpecificConfig(long cfg);

    private String localAddress;
    public String getLocalAddress() { return localAddress; }
    public void setLocalAddress(String la) { localAddress = la; }

    private int maxOutputPausePeriod;
    public int getMaxOutputPausePeriod() { return maxOutputPausePeriod; }
    public void setMaxOutputPausePeriod(int mopp) {
        maxOutputPausePeriod = mopp; 
    }

}
