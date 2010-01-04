/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

import DDS.*;
import OpenDDS.DCPS.*;
import OpenDDS.DCPS.transport.*;
import org.omg.CORBA.StringSeqHolder;
import Messenger.*;

public class DataReaderListenerImpl extends DDS._DataReaderListenerLocalBase {

    private int num_reads_;
    public synchronized int num_reads () { return num_reads_; }

    public synchronized void on_data_available(DDS.DataReader reader) {

        ++num_reads_;
        MessageDataReader mdr = MessageDataReaderHelper.narrow(reader);
        if (mdr == null) {
            System.err.println("ERROR: read: narrow failed.");
            return;
        }

        MessageHolder mh = new MessageHolder(new Message());
        SampleInfoHolder sih = new SampleInfoHolder(new SampleInfo(0, 0, 0,
            new DDS.Time_t(), 0, 0, 0, 0, 0, 0, 0, false));
        int status = mdr.take_next_sample(mh, sih);

        if (status == RETCODE_OK.value) {

            System.out.println("SampleInfo.sample_rank = "
                                + sih.value.sample_rank);
            System.out.println("SampleInfo.instance_state = "
                                + sih.value.instance_state);

          if (sih.value.valid_data) {

              System.out.println("Message: subject    = " + mh.value.subject);
              System.out.println("         subject_id = "
                                 + mh.value.subject_id);
              System.out.println("         from       = " + mh.value.from);
              System.out.println("         count      = " + mh.value.count);
              System.out.println("         text       = " + mh.value.text);
              System.out.println("SampleInfo.sample_rank = "
                                 + sih.value.sample_rank);
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
                                 + "received unknown instance state "
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
}
