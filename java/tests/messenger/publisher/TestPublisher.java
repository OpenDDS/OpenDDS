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

public class TestPublisher {

    private static final int N_MSGS = 40;

    public static void main(String[] args) {

        System.out.println("Start Publisher");

        DomainParticipantFactory dpf =
            TheParticipantFactory.WithArgs(new StringSeqHolder(args));
        if (dpf == null) {
            System.err.println("ERROR: Domain Participant Factory not found");
            return;
        }
        System.out.println("Publisher Created DomainParticipantFactory");
        DomainParticipant dp = dpf.create_participant(4,
            PARTICIPANT_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
        if (dp == null) {
            System.err.println("ERROR: Domain Participant creation failed");
            return;
        }
        System.out.println("Publisher Created DomainParticipant");

        MessageTypeSupportImpl servant = new MessageTypeSupportImpl();
        if (servant.register_type(dp, "") != RETCODE_OK.value) {
            System.err.println("ERROR: register_type failed");
            return;
        }
        System.out.println("Publisher Registered MessageTypeSupportImpl");

        Topic top = dp.create_topic("Movie Discussion List",
                                    servant.get_type_name(),
                                    TOPIC_QOS_DEFAULT.get(),
                                    null,
                                    DEFAULT_STATUS_MASK.value);
        if (top == null) {
            System.err.println("ERROR: Topic creation failed");
            return;
        }
        System.out.println("Publisher Created Topic");

        Publisher pub = dp.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null,
                                            DEFAULT_STATUS_MASK.value);
        if (pub == null) {
            System.err.println("ERROR: Publisher creation failed");
            return;
        }
        System.out.println("Publisher Created Publisher");

        // Use the default transport configuration (do nothing)

        DataWriterQos dw_qos = new DataWriterQos();
        dw_qos.durability = new DurabilityQosPolicy();
        dw_qos.durability_service = new DurabilityServiceQosPolicy();
        dw_qos.durability_service.service_cleanup_delay = new Duration_t();
        dw_qos.deadline = new DeadlineQosPolicy();
        dw_qos.deadline.period = new Duration_t();
        dw_qos.latency_budget = new LatencyBudgetQosPolicy();
        dw_qos.latency_budget.duration = new Duration_t();
        dw_qos.liveliness = new LivelinessQosPolicy();
        dw_qos.liveliness.lease_duration = new Duration_t();
        dw_qos.reliability = new ReliabilityQosPolicy();
        dw_qos.reliability.max_blocking_time = new Duration_t();
        dw_qos.destination_order = new DestinationOrderQosPolicy();
        dw_qos.history = new HistoryQosPolicy();
        dw_qos.resource_limits = new ResourceLimitsQosPolicy();
        dw_qos.transport_priority = new TransportPriorityQosPolicy();
        dw_qos.lifespan = new LifespanQosPolicy();
        dw_qos.lifespan.duration = new Duration_t();
        dw_qos.user_data = new UserDataQosPolicy();
        // dw_qos.user_data.value = new byte[];
        dw_qos.ownership = new OwnershipQosPolicy();
        dw_qos.ownership_strength = new OwnershipStrengthQosPolicy();
        dw_qos.writer_data_lifecycle = new WriterDataLifecycleQosPolicy();
        
        //DataWriterQosHolder qos = new DataWriterQosHolder(dw_qos);
        //DataWriterQosHolder qos = new DataWriterQosHolder();
        //System.out.println("Publisher Get Default DataWriter QOS");
        //pub.get_default_datawriter_qos(qos);
        //System.out.println("Publisher Set KEEP_ALL_HISTORY_QOS");
        //qos.value.history.kind = HistoryQosPolicyKind.KEEP_ALL_HISTORY_QOS;
        System.out.println("Publisher Create DataWriter");
        DataWriter dw = pub.create_datawriter(top,
                                              //qos.value,
                                              DATAWRITER_QOS_DEFAULT.get(),
                                              null,
                                              DEFAULT_STATUS_MASK.value);
        if (dw == null) {
            System.err.println("ERROR: DataWriter creation failed");
            return;
        }
        System.out.println("Publisher Created DataWriter");

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
                System.out.println("Publisher Matched");
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
        int ret = RETCODE_TIMEOUT.value;
        for (; msg.count < N_MSGS; ++msg.count) {
            while ((ret = mdw.write(msg, handle)) == RETCODE_TIMEOUT.value) {
            }
            if (ret != RETCODE_OK.value) {
                System.err.println("ERROR " + msg.count +
                                   " write() returned " + ret);
            }
            try {
              Thread.sleep(100);
            } catch(InterruptedException ex) {
              Thread.currentThread().interrupt();
            }
        }

        System.out.println("Stop Publisher");

        // Clean up
        dp.delete_contained_entities();
        dpf.delete_participant(dp);
        TheServiceParticipant.shutdown();

        System.out.println("Publisher exiting");
    }
}
