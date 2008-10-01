import DDS.*;
import OpenDDS.DCPS.*;
import OpenDDS.DCPS.transport.*;
import org.omg.CORBA.StringSeqHolder;
import Messenger.*;

public class TestPublisher {

    private static final int N_MSGS = 10;

    public static void main(String[] args) {

        DomainParticipantFactory dpf =
            TheParticipantFactory.WithArgs(new StringSeqHolder(args));
        DomainParticipant dp = dpf.create_participant(411,
            PARTICIPANT_QOS_DEFAULT.get(), null);

        MessageTypeSupportImpl servant = new MessageTypeSupportImpl();
        servant.register_type(dp, "");
        Topic top = dp.create_topic("Movie Discussion List",
                                    servant.get_type_name(),
                                    TOPIC_QOS_DEFAULT.get(), null);

        Publisher pub = dp.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null);

        //OpenDDS-specific attachment of transport to publisher
        TransportImpl tcp_impl =
            TheTransportFactory.create_transport_impl(1,
            TheTransportFactory.AUTO_CONFIG);
        AttachStatus stat = tcp_impl.attach_to_publisher(pub);
        if(stat.value() != AttachStatus._ATTACH_OK) {
            System.err.println ("ERROR: Couldn't attach transport.");
            System.exit(1);
        }

        DataWriter dw = pub.create_datawriter(top, DATAWRITER_QOS_DEFAULT.get(),
                                              null);

        InstanceHandleSeqHolder holder =
            new InstanceHandleSeqHolder(new int[]{});
        while(holder.value.length == 0) {
            dw.get_matched_subscriptions(holder);
            if(holder.value.length == 0)
        	try{ Thread.sleep(200); } catch (InterruptedException ie) {}
        }

        MessageDataWriter mdw = MessageDataWriterHelper.narrow(dw);
        Message msg = new Message();
        msg.subject_id = 99;
        int handle = mdw.register(msg);
        msg.from = "OpenDDS-Java";
        msg.subject = "Review";
        msg.text = "Worst. Movie. Ever.";
        msg.count = 0;
        for(; msg.count < N_MSGS; ++msg.count) {
            mdw.write(msg, handle);
        }
        //not synching after write, OK?
        try{ Thread.sleep(3000); } catch (InterruptedException ie) {}

        dp.delete_contained_entities();
        dpf.delete_participant(dp);
        TheTransportFactory.release();
        TheServiceParticipant.shutdown();
    }

}
