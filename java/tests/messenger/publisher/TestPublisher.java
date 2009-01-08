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

        Publisher pub = dp.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null);
        if (pub == null) {
            System.err.println("ERROR: Publisher creation failed");
            return;
        }

        //OpenDDS-specific attachment of transport to publisher
        TransportImpl transport_impl =
            TheTransportFactory.create_transport_impl(1,
                TheTransportFactory.AUTO_CONFIG);
        if (transport_impl == null) {
            System.err.println("ERROR: Transport creation failed");
            return;
        }

        AttachStatus stat = transport_impl.attach_to_publisher(pub);
        if(stat != AttachStatus.ATTACH_OK) {
            System.err.println("ERROR: Couldn't attach transport.");
            System.exit(1);
        }

        DataWriter dw = pub.create_datawriter(top,
                                              DATAWRITER_QOS_DEFAULT.get(),
                                              null);
        if (dw == null) {
            System.err.println("ERROR: DataWriter creation failed");
            return;
        }

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
            int ret = mdw.write(msg, handle);
            if (ret != RETCODE_OK.value) {
                System.err.println("ERROR " + msg.count +
                                   "dth write() returned " + ret);
          }
        }
        //not synching after write, OK?
        try{ Thread.sleep(3000); } catch (InterruptedException ie) {}

        dp.delete_contained_entities();
        dpf.delete_participant(dp);
        TheTransportFactory.release();
        TheServiceParticipant.shutdown();
    }

}
