package OpenDDS.DCPS.transport;

public class SimpleUdpConfiguration
    extends SimpleUnreliableDgramConfiguration {

    SimpleUdpConfiguration(int id) {
        super(id);
    }

    public String getType() { return "SimpleUdp"; }
}
