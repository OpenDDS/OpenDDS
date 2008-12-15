package OpenDDS.DCPS.transport;

public class ReliableMulticastConfiguration extends TransportConfiguration {

    ReliableMulticastConfiguration(int id) {
        super(id);
    }

    native void saveSpecificConfig(long cfg);
    native void loadSpecificConfig(long cfg);

    public String getType() { return "ReliableMulticast"; }

    private String localAddress;
    public String getLocalAddress() { return localAddress; }
    public void setLocalAddress(String la) { localAddress = la; }

    private String multicastGroupAddress;
    public String getMulticastGroupAddress() { return multicastGroupAddress; }
    public void setMulticastGroupAddress(String mga) {
        multicastGroupAddress = mga;
    }

    private boolean receiver;
    public boolean isReceiver() { return receiver; }
    public void setReceiver(boolean r) { receiver = r; }

    private int senderHistorySize;
    public int getSenderHistorySize() { return senderHistorySize; }
    public void setSenderHistorySize(int shs) { senderHistorySize = shs; }

    private int receiverBufferSize;
    public int getReceiverBufferSize() { return receiverBufferSize; }
    public void setReceiverBufferSize(int rbs) { receiverBufferSize = rbs; }

}
