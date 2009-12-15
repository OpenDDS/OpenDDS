/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

import org.omg.CORBA.StringSeqHolder;
import org.omg.CORBA.SystemException;

import OpenDDS.DCPS.TheServiceParticipant;
import OpenDDS.DCPS.TheParticipantFactory;

import OpenDDS.DCPS.transport.TheTransportFactory;
import OpenDDS.DCPS.transport.TransportImpl;
import OpenDDS.DCPS.transport.TransportException;
import OpenDDS.DCPS.transport.TransportConfiguration;
import OpenDDS.DCPS.transport.SimpleTcpConfiguration;
import OpenDDS.DCPS.transport.SimpleUdpConfiguration;
import OpenDDS.DCPS.transport.MulticastConfiguration;

public class TransportConfigTest {

    protected static void setUp(String[] args) {

        // For debugging:
        //    OpenDDS.DCPS.TheParticipantFactory.loadNativeLib();
        //    System.out.println("READY");
        //    try {System.in.read();} catch (java.io.IOException ioe) {}

        // We need to load the SimpleTcp lib before initializing the
        // ParticipantFactory, because it will read and parse the config file.
        TheTransportFactory.get_or_create_configuration(99 /*bogus ID*/,
            TheTransportFactory.TRANSPORT_TCP);
        TheParticipantFactory.WithArgs(new StringSeqHolder(args));
    }


    public static void main(String[] args) throws Exception {
        setUp(args);
        
        testModifyTransportFromFileTCP();
        testCreateNewTransportUDP();
        testCreateNewTransportMulticast();

        tearDown();
    }


    protected static void testModifyTransportFromFileTCP() throws Exception {
        final int ID = 1; //matches .ini file
        TransportConfiguration tc =
            TheTransportFactory.get_or_create_configuration(ID,
                TheTransportFactory.TRANSPORT_TCP);
        // Verify the values were read in from the ini file
        assert tc.isSwapBytes();
        assert tc.getMaxSamplesPerPacket() == 5;
        SimpleTcpConfiguration tc_tcp = (SimpleTcpConfiguration) tc;
        assert tc_tcp.getConnRetryAttempts() == 42;

        tc.setSwapBytes(false);

        TransportImpl ti = TheTransportFactory.create_transport_impl(ID,
            TheTransportFactory.TRANSPORT_TCP,
            TheTransportFactory.DONT_AUTO_CONFIG);
        ti.configure(tc);

        tc = TheTransportFactory.get_or_create_configuration(ID,
                 TheTransportFactory.TRANSPORT_TCP);
        assert !tc.isSwapBytes();
        assert tc.getMaxSamplesPerPacket() == 5;

        TheTransportFactory.release(ID);
    }


    protected static void testCreateNewTransportUDP() throws Exception {
        final int ID = 2;
        TransportConfiguration tc =
            TheTransportFactory.get_or_create_configuration(ID,
                 TheTransportFactory.TRANSPORT_UDP_UNI);
        tc.setSendThreadStrategy(TransportConfiguration.ThreadSynchStrategy.NULL_SYNCH);
        SimpleUdpConfiguration suc = (SimpleUdpConfiguration) tc;
        suc.setLocalAddress("0.0.0.0:2047");
        suc.setMaxOutputPausePeriod(123456);

        TransportImpl ti = TheTransportFactory.create_transport_impl(ID,
            TheTransportFactory.TRANSPORT_UDP_UNI,
            TheTransportFactory.DONT_AUTO_CONFIG);
        ti.configure(tc);

        tc = TheTransportFactory.get_or_create_configuration(ID,
                 TheTransportFactory.TRANSPORT_UDP_UNI);
        assert tc.getSendThreadStrategy() == TransportConfiguration.ThreadSynchStrategy.NULL_SYNCH;
        suc = (SimpleUdpConfiguration) tc;
        //Only checking endsWith here b/c the 0.0.0.0 is resolved to a hostname
        assert suc.getLocalAddress().endsWith(":2047");
        assert suc.getMaxOutputPausePeriod() == 123456;

        TheTransportFactory.release(ID);
    }


    protected static void testCreateNewTransportMulticast() throws Exception {
        final int ID = 3;
        TransportConfiguration tc =
            TheTransportFactory.get_or_create_configuration(ID,
                 TheTransportFactory.TRANSPORT_MULTICAST);
        tc.setSendThreadStrategy(TransportConfiguration.ThreadSynchStrategy.NULL_SYNCH);
        MulticastConfiguration mc = (MulticastConfiguration) tc;
        mc.setDefaultToIPv6(true);
        mc.setPortOffset((short) 9000);
        mc.setGroupAddress("224.0.0.1:1234");
        mc.setReliable(false);
	mc.setSynBackoff(0.5);
        mc.setSynInterval(100);
        mc.setSynTimeout(100);
        mc.setNakDepth(16);
        mc.setNakInterval(100);
        mc.setNakTimeout(100);

        TransportImpl ti = TheTransportFactory.create_transport_impl(ID,
            TheTransportFactory.TRANSPORT_MULTICAST,
            TheTransportFactory.DONT_AUTO_CONFIG);
        ti.configure(tc);

        tc = TheTransportFactory.get_or_create_configuration(ID,
                 TheTransportFactory.TRANSPORT_MULTICAST);
        assert tc.getSendThreadStrategy() == TransportConfiguration.ThreadSynchStrategy.NULL_SYNCH;
        mc = (MulticastConfiguration) tc;
        assert mc.getDefaultToIPv6() == true;
        assert mc.getPortOffset() == 9000;
        assert mc.getGroupAddress().equals("224.0.0.1:1234");
        assert mc.getReliable() == false;
	assert mc.getSynBackoff() == 0.5;
        assert mc.getSynInterval() == 100;
        assert mc.getSynTimeout() == 100;
        assert mc.getNakDepth() == 16;
        assert mc.getNakInterval() == 100;
        assert mc.getNakTimeout() == 100;

        TheTransportFactory.release(ID);
    }


    protected static void tearDown() {
        TheTransportFactory.release();
        TheServiceParticipant.shutdown();
    }
}
