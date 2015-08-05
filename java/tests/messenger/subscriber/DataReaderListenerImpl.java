/*
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
import java.util.ArrayList;

public class DataReaderListenerImpl extends DDS._DataReaderListenerLocalBase {

    private int num_msgs = 0;

    private static final int N_EXPECTED = 40;
    private ArrayList<Boolean> counts = new ArrayList<Boolean>(N_EXPECTED);

    private void initialize_counts() {
        if (counts.size() > 0) {
          return;
        }

        for (int i = 0; i < N_EXPECTED; ++i) {
            counts.add(false);
        }
    }

    public synchronized void on_data_available(DDS.DataReader reader) {

        initialize_counts();

        MessageDataReader mdr = MessageDataReaderHelper.narrow(reader);
        if (mdr == null) {
            System.err.println("ERROR: read: narrow failed.");
            return;
        }

        MessageHolder mh = new MessageHolder(new Message());
        SampleInfoHolder sih = new SampleInfoHolder(new SampleInfo(0, 0, 0,
            new DDS.Time_t(), 0, 0, 0, 0, 0, 0, 0, false, 0));
        int status = mdr.take_next_sample(mh, sih);

        if (status == RETCODE_OK.value) {

            System.out.println("SampleInfo.sample_rank = "
                                + sih.value.sample_rank);
            System.out.println("SampleInfo.instance_state = "
                                + sih.value.instance_state);

            if (sih.value.valid_data) {

                String prefix = "";
                boolean invalid_count = false;
                if (mh.value.count < 0 || mh.value.count >= counts.size()) {
                    invalid_count = true;
                }
                else {
                    if (counts.get(mh.value.count) == false){
                        counts.set(mh.value.count, true);
                    }
                    else {
                        prefix = "ERROR: Repeat ";
                    }
                }
                System.out.println(prefix + "Message: subject    = " + mh.value.subject);
                System.out.println("         subject_id = "
                                   + mh.value.subject_id);
                System.out.println("         from       = " + mh.value.from);
                System.out.println("         count      = " + mh.value.count);
                System.out.println("         text       = " + mh.value.text);
                System.out.println("SampleInfo.sample_rank = "
                                   + sih.value.sample_rank);

                if (invalid_count == true) {
                    System.out.println("ERROR: Invalid message.count (" + mh.value.count + ")");
                }
                if (!mh.value.from.equals("Comic Book Guy") && !mh.value.from.equals("OpenDDS-Java")) {
                    System.out.println("ERROR: Invalid message.from (" + mh.value.from + ")");
                }
                if (!mh.value.subject.equals("Review")) {
                    System.out.println("ERROR: Invalid message.subject (" + mh.value.subject + ")");
                }
                if (!mh.value.text.equals("Worst. Movie. Ever.")) {
                    System.out.println("ERROR: Invalid message.text (" + mh.value.text + ")");
                }
                if (mh.value.subject_id != 99) {
                    System.out.println("ERROR: Invalid message.subject_id (" + mh.value.subject_id + ")");
                }
            }
            else if (sih.value.instance_state ==
                     NOT_ALIVE_DISPOSED_INSTANCE_STATE.value) {
                System.out.println("instance is disposed");
            }
            else if (sih.value.instance_state ==
                     NOT_ALIVE_NO_WRITERS_INSTANCE_STATE.value) {
                System.out.println("instance is unregistered");
            }
            else {
                System.out.println("DataReaderListenerImpl::on_data_available: "
                                   + "ERROR: received unknown instance state "
                                   + sih.value.instance_state);
            }

        } else if (status == RETCODE_NO_DATA.value) {
            System.err.println("ERROR: reader received DDS::RETCODE_NO_DATA!");
        } else {
            System.err.println("ERROR: read Message: Error: " + status);
        }
    }

    public void on_requested_deadline_missed(DDS.DataReader reader, DDS.RequestedDeadlineMissedStatus status) {
        System.err.println("DataReaderListenerImpl.on_requested_deadline_missed");
    }

    public void on_requested_incompatible_qos(DDS.DataReader reader, DDS.RequestedIncompatibleQosStatus status) {
        System.err.println("DataReaderListenerImpl.on_requested_incompatible_qos");
    }

    public void on_sample_rejected(DDS.DataReader reader, DDS.SampleRejectedStatus status) {
        System.err.println("DataReaderListenerImpl.on_sample_rejected");
    }

    public void on_liveliness_changed(DDS.DataReader reader, DDS.LivelinessChangedStatus status) {
        System.err.println("DataReaderListenerImpl.on_liveliness_changed");
    }

    public void on_subscription_matched(DDS.DataReader reader, DDS.SubscriptionMatchedStatus status) {
        System.err.println("DataReaderListenerImpl.on_subscription_matched");
    }

    public void on_sample_lost(DDS.DataReader reader, DDS.SampleLostStatus status) {
        System.err.println("DataReaderListenerImpl.on_sample_lost");
    }

    public void report_validity() {
        int count = 0;
        int missed_counts = 0;
        for (Boolean val : counts) {
            if (val == false)
                ++missed_counts;
        }
        if (missed_counts > 0) {
            System.out.println("ERROR: Missing " + missed_counts + " messages");
        }
    }
}
