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
        if (dpf == null) {
            System.err.println("ERROR: Domain Participant Factory not found");
            return;
        }
        DomainParticipant dp = dpf.create_participant(411,
            PARTICIPANT_QOS_DEFAULT.get(), null);
        if (dp == null) {
            System.err.println("ERROR: Domain Participant creation failed");
            return;
        }

        MessageTypeSupportImpl servant = new MessageTypeSupportImpl();
        if (servant.register_type(dp, "") != RETCODE_OK.value) {
            System.err.println("ERROR: register_type failed");
            return;
        }
        Topic top = dp.create_topic("Movie Discussion List",
                                    servant.get_type_name(),
                                    TOPIC_QOS_DEFAULT.get(), null);
        if (top == null) {
            System.err.println("ERROR: Topic creation failed");
            return;
        }

        Subscriber sub = dp.create_subscriber(SUBSCRIBER_QOS_DEFAULT.get(),
                                              null);
        if (sub == null) {
            System.err.println("ERROR: Subscriber creation failed");
            return;
        }

        //OpenDDS-specific attachment of transport to subscriber
        TransportImpl transport_impl =
            TheTransportFactory.create_transport_impl(1,
                TheTransportFactory.AUTO_CONFIG);
        if (transport_impl == null) {
            System.err.println("ERROR: Transport creation failed");
            return;
        }

        AttachStatus stat = transport_impl.attach_to_subscriber(sub);
        if(stat != AttachStatus.ATTACH_OK) {
            System.err.println("ERROR: Couldn't attach transport.");
            System.exit(1);
        }

        DataReaderListenerImpl listener = new DataReaderListenerImpl();
        DataReader dr = sub.create_datareader(top,
                                              DATAREADER_QOS_DEFAULT.get(),
                                              listener);
        if (dr == null) {
            System.err.println("ERROR: DataReader creation failed");
            return;
        }

        while(listener.num_reads () < N_EXPECTED)
            try { Thread.sleep(1000); } catch (InterruptedException ie) {}

        dp.delete_contained_entities();
        dpf.delete_participant(dp);
        TheTransportFactory.release();
        TheServiceParticipant.shutdown();
    }

}
