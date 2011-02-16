/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

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

        Topic top = dp.create_topic("Movie Discussion List",
                                    servant.get_type_name(),
                                    TOPIC_QOS_DEFAULT.get(),
                                    null,
                                    DEFAULT_STATUS_MASK.value);
        if (top == null) {
            System.err.println("ERROR: Topic creation failed");
            return;
        }

        Publisher pub = dp.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null,
                                            DEFAULT_STATUS_MASK.value);
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
                                              null,
                                              DEFAULT_STATUS_MASK.value);
        if (dw == null) {
            System.err.println("ERROR: DataWriter creation failed");
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

        // Wait for samples to be acknowledged
        if (dw.wait_for_acknowledgments(timeout) != RETCODE_OK.value) {
            System.err.println("ERROR: wait_for_acknowledgments failed!");
            return;
        }

        // Clean up
        dp.delete_contained_entities();
        dpf.delete_participant(dp);
        TheTransportFactory.release();
        TheServiceParticipant.shutdown();
    }
}
