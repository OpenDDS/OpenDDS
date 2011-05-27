import DDS.*;
import OpenDDS.DCPS.*;
import OpenDDS.DCPS.transport.*;
import org.omg.CORBA.StringSeqHolder;
import Messenger.*;

public class TestSubscriber {

    private static final int N_EXPECTED = 10;

    public static void main(String[] args) throws Exception {
        DomainParticipantFactory dpf =
            TheParticipantFactory.WithArgs(new StringSeqHolder(args));
        DomainParticipant dp = dpf.create_participant(411,
            PARTICIPANT_QOS_DEFAULT.get(), null);

        MessageTypeSupportImpl servant = new MessageTypeSupportImpl();
        servant.register_type(dp, "");
        Topic top = dp.create_topic("Movie Discussion List",
                                    servant.get_type_name(),
                                    TOPIC_QOS_DEFAULT.get(), null);

        Subscriber sub = dp.create_subscriber(SUBSCRIBER_QOS_DEFAULT.get(),
                                              null);

        //OpenDDS-specific attachment of transport to subscriber
        TransportImpl tcp_impl =
            TheTransportFactory.create_transport_impl(1,
            TheTransportFactory.AUTO_CONFIG);
        AttachStatus stat = tcp_impl.attach_to_subscriber(sub);
        if(stat.value() != AttachStatus._ATTACH_OK) {
            System.err.println ("ERROR: Couldn't attach transport.");
            System.exit(1);
        }

        DataReaderListenerImpl listener = new DataReaderListenerImpl();
        sub.create_datareader(top, DATAREADER_QOS_DEFAULT.get(), listener);

        while(listener.num_reads () < N_EXPECTED)
            try { Thread.sleep(1000); } catch (InterruptedException ie) {}

        dp.delete_contained_entities();
        dpf.delete_participant(dp);
        TheTransportFactory.release();
        TheServiceParticipant.shutdown();
    }

}
