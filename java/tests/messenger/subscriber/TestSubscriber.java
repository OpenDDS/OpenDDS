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

    public static void main(String[] args) throws Exception {

        System.out.println("Start Subscriber");

        DomainParticipantFactory dpf =
            TheParticipantFactory.WithArgs(new StringSeqHolder(args));
        if (dpf == null) {
            System.err.println("ERROR: Domain Participant Factory not found");
            return;
        }
        System.out.println("Subscriber Created DomainParticipantFactory");
        DomainParticipant dp = dpf.create_participant(4,
            PARTICIPANT_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
        if (dp == null) {
            System.err.println("ERROR: Domain Participant creation failed");
            return;
        }
        System.out.println("Subscriber Created DomainParticipant");

        MessageTypeSupportImpl servant = new MessageTypeSupportImpl();
        if (servant.register_type(dp, "") != RETCODE_OK.value) {
            System.err.println("ERROR: register_type failed");
            return;
        }
        System.out.println("Subscriber Registered MessageTypeSupportImpl");
        Topic top = dp.create_topic("Movie Discussion List",
                                    servant.get_type_name(),
                                    TOPIC_QOS_DEFAULT.get(),
                                    null,
                                    DEFAULT_STATUS_MASK.value);
        if (top == null) {
            System.err.println("ERROR: Topic creation failed");
            return;
        }
        System.out.println("Subscriber Created Topic");

        Subscriber sub = dp.create_subscriber(SUBSCRIBER_QOS_DEFAULT.get(),
                                              null, DEFAULT_STATUS_MASK.value);
        if (sub == null) {
            System.err.println("ERROR: Subscriber creation failed");
            return;
        }
        System.out.println("Subscriber Created Subscriber");

        // Use the default transport (do nothing)

        DataReaderQosHolder qos = new DataReaderQosHolder(new DataReaderQos());
        System.out.println("Subscriber Get Default DataReader QOS");
        sub.get_default_datareader_qos(qos);
        System.out.println("Subscriber Set KEEP_ALL_HISTORY_QOS");
        qos.value.liveliness.kind = LivelinessQosPolicyKind.AUTOMATIC_LIVELINESS_QOS;
        qos.value.liveliness.lease_duration.sec = 10;
        qos.value.liveliness.lease_duration.nanosec = 0;
        qos.value.history.kind = HistoryQosPolicyKind.KEEP_ALL_HISTORY_QOS;
        DataReaderListenerImpl listener = new DataReaderListenerImpl();
        System.out.println("Subscriber Create DataReader");
        DataReader dr = sub.create_datareader(top,
                                              qos.value,
                                              listener,
                                              DEFAULT_STATUS_MASK.value);
        if (dr == null) {
            System.err.println("ERROR: DataReader creation failed");
            return;
        }
        System.out.println("Subscriber Created DataReader");

        StatusCondition sc = dr.get_statuscondition();
        sc.set_enabled_statuses(SUBSCRIPTION_MATCHED_STATUS.value);
        WaitSet ws = new WaitSet();
        ws.attach_condition(sc);
        SubscriptionMatchedStatusHolder matched =
          new SubscriptionMatchedStatusHolder(new SubscriptionMatchedStatus());
        Duration_t timeout = new Duration_t(DURATION_INFINITE_SEC.value,
                                            DURATION_INFINITE_NSEC.value);

        boolean matched_pub = false;
        while (true) {
            final int result = dr.get_subscription_matched_status(matched);
            if (result != RETCODE_OK.value) {
                System.err.println("ERROR: get_subscription_matched_status()" +
                                   "failed.");
                return;
            }

            if (matched.value.current_count == 0
                && matched.value.total_count > 0) {
                System.out.println("Subscriber No Longer Matched");
                break;
            }
            else if (matched.value.current_count > 0 &&
                     !matched_pub) {
                System.out.println("Subscriber Matched");
                matched_pub = true;
            }

            ConditionSeqHolder cond = new ConditionSeqHolder(new Condition[]{});
            if (ws.wait(cond, timeout) != RETCODE_OK.value) {
                System.err.println("ERROR: wait() failed.");
                return;
            }
        }

        System.out.println("Subscriber Report Validity");
        listener.report_validity();

        ws.detach_condition(sc);

        System.out.println("Stop Subscriber");

        dp.delete_contained_entities();
        dpf.delete_participant(dp);
        TheServiceParticipant.shutdown();

        System.out.println("Subscriber exiting");
    }
}
