import DDS.*;
import OpenDDS.DCPS.*;
import org.omg.CORBA.StringSeqHolder;
import Messenger.*;

public class ZeroCopy {
    private static final int N_MSGS = 10;

    public static void main(String[] args) {

        DomainParticipantFactory dpf =
            TheParticipantFactory.WithArgs(new StringSeqHolder(args));
        if (dpf == null) {
            System.err.println("ERROR: Domain Participant Factory not found");
            return;
        }
        DomainParticipant dp = dpf.create_participant(4,
            PARTICIPANT_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
        if (dp == null) {
            System.err.println("ERROR: Domain Participant creation failed");
            return;
        }

        MessageTypeSupportImpl servant = new MessageTypeSupportImpl();
        if (servant.register_type(dp, "") != RETCODE_OK.value) {
            System.err.println("ERROR: register_type failed");
            return;
        }

        Publisher pub = dp.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null,
                                            DEFAULT_STATUS_MASK.value);
        if (pub == null) {
            System.err.println("ERROR: Publisher creation failed");
            return;
        }

        Subscriber sub = dp.create_subscriber(SUBSCRIBER_QOS_DEFAULT.get(),
                                              null, DEFAULT_STATUS_MASK.value);
        if (sub == null) {
            System.err.println("ERROR: Subscriber creation failed");
            return;
        }

        Topic top = dp.create_topic("Movie Discussion List",
                                    servant.get_type_name(),
                                    TOPIC_QOS_DEFAULT.get(),
                                    null,
                                    DEFAULT_STATUS_MASK.value);
        if (top == null) {
            System.err.println("ERROR: Topic creation failed");
            return;
        }

        DataWriter dw = pub.create_datawriter(top,
                                              DATAWRITER_QOS_DEFAULT.get(),
                                              null,
                                              DEFAULT_STATUS_MASK.value);
        if (dw == null) {
            System.err.println("ERROR: DataWriter creation failed");
            return;
        }

        DataReader dr = sub.create_datareader(top,
                                              DATAREADER_QOS_DEFAULT.get(),
                                              null,
                                              DEFAULT_STATUS_MASK.value);
        if (dr == null) {
            System.err.println("ERROR: DataReader creation failed");
            return;
        }

        StatusCondition sc = dw.get_statuscondition();
        sc.set_enabled_statuses(PUBLICATION_MATCHED_STATUS.value);
        WaitSet ws = new WaitSet();
        ws.attach_condition(sc);
        PublicationMatchedStatusHolder matched =
            new PublicationMatchedStatusHolder(new PublicationMatchedStatus());
        Duration_t timeout = new Duration_t(DURATION_INFINITE_SEC.value,
                                            DURATION_INFINITE_NSEC.value);

        while (true) {
            final int result = dw.get_publication_matched_status(matched);
            if (result != RETCODE_OK.value) {
                System.err.println("ERROR: get_publication_matched_status()" +
                                   "failed.");
                return;
            }

            if (matched.value.current_count >= 1) {
                break;
            }

            ConditionSeqHolder cond = new ConditionSeqHolder(new Condition[]{});
            if (ws.wait(cond, timeout) != RETCODE_OK.value) {
                System.err.println("ERROR: wait() failed.");
                return;
            }
        }

        ws.detach_condition(sc);

        MessageDataWriter mdw = MessageDataWriterHelper.narrow(dw);
        Message msg = new Message();
        msg.subject_id = 99;
        int handle = mdw.register_instance(msg);
        msg.from = "OpenDDS-Java";
        msg.subject = "Review";
        msg.text = "Worst. Movie. Ever.";
        msg.count = 0;
        for (; msg.count < N_MSGS; ++msg.count) {
            int ret = mdw.write(msg, handle);
            if (ret != RETCODE_OK.value) {
                System.err.println("ERROR " + msg.count +
                                   "dth write() returned " + ret);
          }
        }
        mdw.dispose(msg, handle);

        MessageDataReader mdr = MessageDataReaderHelper.narrow(dr);
        ReadCondition rc = dr.create_readcondition(ANY_SAMPLE_STATE.value,
                                                   ANY_VIEW_STATE.value,
                                                   ANY_INSTANCE_STATE.value);
        ws.attach_condition(rc);
        boolean done = false;
        while (!done) {
            ConditionSeqHolder cond = new ConditionSeqHolder(new Condition[]{});
            if (ws.wait(cond, timeout) != RETCODE_OK.value) {
                System.err.println("ERROR: DataReader wait() failed.");
                return;
            }

            MessageSeqHolder mholder = new MessageSeqHolder(new Message[]{});
            SampleInfoSeqHolder infoholder =
                new SampleInfoSeqHolder(new SampleInfo[]{});
            if (mdr.take_w_condition(mholder, infoholder,
                                     LENGTH_UNLIMITED.value, rc)
                != RETCODE_OK.value) {
                System.err.println("ERROR: take_w_condition");
                return;
            }

            for (int i = 0; i < infoholder.value.length; ++i) {
                if (infoholder.value[i].valid_data) {
                    System.out.println("Received " + mholder.value[i].count);
                } else if (infoholder.value[i].instance_state ==
                           NOT_ALIVE_DISPOSED_INSTANCE_STATE.value) {
                    System.out.println("Got 'disposed'");
                    done = true;
                }
            }
        }
        ws.detach_condition(rc);
        dr.delete_readcondition(rc);

        timeout.sec = 10;
        timeout.nanosec = 0;
        int ret = dw.wait_for_acknowledgments(timeout);
        if (ret != RETCODE_OK.value && ret != RETCODE_TIMEOUT.value) {
            System.err.println("ERROR: wait_for_acknowledgments failed!");
            return;
        }

        // Clean up
        dp.delete_contained_entities();
        dpf.delete_participant(dp);
        TheServiceParticipant.shutdown();
    }

}
