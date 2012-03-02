/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

import DDS.*;
import OpenDDS.DCPS.*;
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
        Topic top = dp.create_topic("Movie Discussion List",
                                    servant.get_type_name(),
                                    TOPIC_QOS_DEFAULT.get(),
                                    null,
                                    DEFAULT_STATUS_MASK.value);
        if (top == null) {
            System.err.println("ERROR: Topic creation failed");
            return;
        }

        Subscriber sub = dp.create_subscriber(SUBSCRIBER_QOS_DEFAULT.get(),
                                              null, DEFAULT_STATUS_MASK.value);
        if (sub == null) {
            System.err.println("ERROR: Subscriber creation failed");
            return;
        }

        // Use the default transport (do nothing)

        DataReaderListenerImpl listener = new DataReaderListenerImpl();
        DataReader dr = sub.create_datareader(top,
                                              DATAREADER_QOS_DEFAULT.get(),
                                              listener,
                                              DEFAULT_STATUS_MASK.value);
        if (dr == null) {
            System.err.println("ERROR: DataReader creation failed");
            return;
        }

        StatusCondition sc = dr.get_statuscondition();
        sc.set_enabled_statuses(SUBSCRIPTION_MATCHED_STATUS.value);
        WaitSet ws = new WaitSet();
        ws.attach_condition(sc);
        SubscriptionMatchedStatusHolder matched =
          new SubscriptionMatchedStatusHolder(new SubscriptionMatchedStatus());
        Duration_t timeout = new Duration_t(DURATION_INFINITE_SEC.value,
                                            DURATION_INFINITE_NSEC.value);

        while (true) {
            final int result = dr.get_subscription_matched_status(matched);
            if (result != RETCODE_OK.value) {
                System.err.println("ERROR: get_subscription_matched_status()" +
                                   "failed.");
                return;
            }

            if (matched.value.current_count == 0
                && matched.value.total_count > 0) {
                break;
            }

            ConditionSeqHolder cond = new ConditionSeqHolder(new Condition[]{});
            if (ws.wait(cond, timeout) != RETCODE_OK.value) {
                System.err.println("ERROR: wait() failed.");
                return;
            }
        }

        ws.detach_condition(sc);

        dp.delete_contained_entities();
        dpf.delete_participant(dp);
        TheServiceParticipant.shutdown();
    }
}
