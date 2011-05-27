package OpenDDS.DCPS.transport;

import java.io.Serializable;

public abstract class TransportConfiguration implements Serializable {

    private int id;

    TransportConfiguration(int id_param) {
        id = id_param;
    }

    abstract void saveSpecificConfig(long cfg);
    abstract void loadSpecificConfig(long cfg);

    public static enum ThreadSynchStrategy {
        PER_CONNECTION_SYNCH, //default
        POOL_SYNCH,
        NULL_SYNCH
    }

    public int getId() { return id; }
    
    public abstract String getType();

    private ThreadSynchStrategy sendThreadStrategy;
    public ThreadSynchStrategy getSendThreadStrategy() {
        return sendThreadStrategy;
    }
    public void setSendThreadStrategy(ThreadSynchStrategy sts) {
        sendThreadStrategy = sts;
    }

    private boolean swapBytes;
    public boolean isSwapBytes() { return swapBytes; }
    public void setSwapBytes(boolean swap) { swapBytes = swap; }

    private int queueMessagesPerPool;
    public int getQueueMessagesPerPool() { return queueMessagesPerPool; }
    public void setQueueMessagesPerPool(int qmpp) {
        queueMessagesPerPool = qmpp;
    }

    private int queueInitialPools;
    public int getQueueInitialPools() { return queueInitialPools; }
    public void setQueueInitialPools(int qip) { queueInitialPools = qip; }

    private int maxPacketSize;
    public int getMaxPacketSize() { return maxPacketSize; }
    public void setMaxPacketSize(int mps) { maxPacketSize = mps; }

    private int maxSamplesPerPacket;
    public int getMaxSamplesPerPacket() { return maxSamplesPerPacket; }
    public void setMaxSamplesPerPacket(int mspp) {
        maxSamplesPerPacket = mspp;
    }

    private int optimumPacketSize;
    public int getOptimumPacketSize() { return optimumPacketSize; }
    public void setOptimumPacketSize(int ops) { optimumPacketSize = ops; }

    private boolean threadPerConnection;
    public boolean isTreadPerConnection() { return threadPerConnection; }
    public void getThreadPerConnection(boolean tpc) {
        threadPerConnection = tpc;
    }

    private int datalinkReleaseDelay;
    public int getDatalinkReleaseDelay() { return datalinkReleaseDelay; }
    public void setDatalinkReleaseDelay(int drd) {
        datalinkReleaseDelay = drd;
    }

}
