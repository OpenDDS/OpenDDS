/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

import org.omg.CORBA.StringSeqHolder;
import org.omg.CORBA.SystemException;

import OpenDDS.DCPS.TheServiceParticipant;
import OpenDDS.DCPS.TheParticipantFactory;

import OpenDDS.DCPS.transport.TheTransportRegistry;
import OpenDDS.DCPS.transport.TransportConfig;
import OpenDDS.DCPS.transport.TransportException;
import OpenDDS.DCPS.transport.TransportInst;
import OpenDDS.DCPS.transport.TcpInst;
import OpenDDS.DCPS.transport.UdpInst;
import OpenDDS.DCPS.transport.MulticastInst;

public class TransportConfigTest {

    protected static void setUp(String[] args) {

        // For debugging:
        //    OpenDDS.DCPS.TheParticipantFactory.loadNativeLib();
        //    System.out.println("READY");
        //    try {System.in.read();} catch (java.io.IOException ioe) {}
        TheParticipantFactory.WithArgs(new StringSeqHolder(args));
    }


    public static void main(String[] args) throws Exception {
        setUp(args);

        testModifyTransportFromFileTCP();
        testCreateNewTransportUdp();
        testCreateNewTransportMulticast();
        testConfigCreation();

        tearDown();
    }


    protected static void testModifyTransportFromFileTCP() throws Exception {
        final String ID = "tcp1"; //matches .ini file
        TransportInst ti = TheTransportRegistry.get_inst(ID);
        // Verify the values were read in from the ini file
        assert ti.getMaxSamplesPerPacket() == 5;
        TcpInst ti_tcp = (TcpInst) ti;
        assert ti_tcp.getConnRetryAttempts() == 42;

        ti.setMaxSamplesPerPacket(6);
        ti_tcp.setConnRetryAttempts(49);

        TransportInst ti2 = TheTransportRegistry.get_inst(ID);
        assert ti2.getMaxSamplesPerPacket() == 6;
        TcpInst ti_tcp2 = (TcpInst) ti2;
        assert ti_tcp2.getConnRetryAttempts() == 49;
    }


    protected static void testCreateNewTransportUdp() throws Exception {
        final String ID = "Udp2";
        TransportInst ti =
            TheTransportRegistry.create_inst(ID,
                                             TheTransportRegistry.TRANSPORT_UDP);
        ti.setMaxPacketSize(999);
        UdpInst sui = (UdpInst) ti;
        sui.setLocalAddress("0.0.0.0:1234");

        TransportInst ti2 = TheTransportRegistry.get_inst(ID);
        assert ti2.getMaxPacketSize() == 999;
        UdpInst sui2 = (UdpInst) ti2;
        //Only checking endsWith here b/c the 0.0.0.0 is resolved to a hostname
        assert sui2.getLocalAddress().endsWith(":1234");
    }


    protected static void testCreateNewTransportMulticast() throws Exception {
        final String ID = "multicast3";
        TransportInst ti =
            TheTransportRegistry.create_inst(ID,
                                             TheTransportRegistry.TRANSPORT_MULTICAST);
        ti.setOptimumPacketSize(999);
        MulticastInst mi = (MulticastInst) ti;
        mi.setDefaultToIPv6(true);
        mi.setPortOffset((short) 9000);
        mi.setGroupAddress("224.0.0.1:1234");
        mi.setReliable(false);
        mi.setSynBackoff(0.5);
        mi.setSynInterval(100);
        mi.setSynTimeout(100);
        mi.setNakDepth(16);
        mi.setNakInterval(100);
        mi.setNakDelayInterval(123);
        mi.setNakMax(5);
        mi.setNakTimeout(100);
        mi.setTimeToLive((byte) 21);
        mi.setRcvBufferSize(1023);

        TransportInst ti2 = TheTransportRegistry.get_inst(ID);
        assert ti2.getOptimumPacketSize() == 999;
        MulticastInst mi2 = (MulticastInst) ti2;
        assert mi2.getDefaultToIPv6() == true;
        assert mi2.getPortOffset() == 9000;
        assert mi2.getGroupAddress().equals("224.0.0.1:1234");
        assert mi2.getReliable() == false;
        assert mi2.getSynBackoff() == 0.5;
        assert mi2.getSynInterval() == 100;
        assert mi2.getSynTimeout() == 100;
        assert mi2.getNakDepth() == 16;
        assert mi2.getNakInterval() == 100;
        assert mi2.getNakDelayInterval() == 123;
        assert mi2.getNakMax() == 5;
        assert mi2.getNakTimeout() == 100;
        assert mi2.getTimeToLive() == 21;
        assert mi2.getRcvBufferSize() == 1023;
    }

    protected static void testConfigCreation() throws Exception {
        TransportConfig transport_config =
            TheTransportRegistry.create_config("Config");
        assert (transport_config != null);

        TransportInst transport_inst =
            TheTransportRegistry.create_inst("mytcp",
                                             TheTransportRegistry.TRANSPORT_TCP);
        assert (transport_inst != null);

        transport_config.addLast(transport_inst);
        transport_config.setSwapBytes(true);
        transport_config.setPassiveConnectDuration(20000);
        assert (transport_config.countInstances() == 1);
        assert (transport_config.getInstance(0).getName().equals("mytcp"));
        assert (transport_config.getSwapBytes());
        assert (transport_config.getPassiveConnectDuration() == 20000);
    }

    protected static void tearDown() {
        TheServiceParticipant.shutdown();
    }
}
