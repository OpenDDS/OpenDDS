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
import OpenDDS.DCPS.ConfigStore;

import OpenDDS.DCPS.transport.TheTransportRegistry;
import OpenDDS.DCPS.transport.TransportConfig;
import OpenDDS.DCPS.transport.TransportException;
import OpenDDS.DCPS.transport.TransportInst;
import OpenDDS.DCPS.transport.TcpInst;

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

        testConfigStore();
        testModifyTransportFromFileTCP();
        testConfigCreation();

        tearDown();
    }

    protected static void testConfigStore() throws Exception {
        ConfigStore cs = TheServiceParticipant.config_store();
        assert cs.get_string("TRANSPORT_TCP1_TRANSPORT_TYPE", "").equals("tcp");
        assert cs.get_uint32("TRANSPORT_TCP1_MAX_SAMPLES_PER_PACKET", 0) == 5;
        assert cs.get_uint32("TRANSPORT_TCP1_CONN_RETRY_ATTEMPTS", 0) == 42;
        cs.set_string("MY_KEY", "value");
        assert cs.get_string("MY_KEY", "").equals("value");
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
