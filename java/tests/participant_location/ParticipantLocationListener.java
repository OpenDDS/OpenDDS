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


public class ParticipantLocationListener extends DDS._DataReaderListenerLocalBase {

    public ParticipantLocationListener() {
      System.out.println("creating participant listener");
    }

    public synchronized void on_data_available(DDS.DataReader reader) {
      System.out.println("on_data_available");
/*
      // 1.  Narrow the DataReader to an ParticipantLocationBuiltinTopicDataDataReader
      // 2.  Read the samples from the data reader
      // 3.  Print out the contents of the samples
      ParticipantLocationBuiltinTopicDataDataReader bitDataReader =
        ParticipantLocationBuiltinTopicDataDataReaderHelper.narrow(reader);

      if (bitDataReader == null)
      {
        System.err.println("ParticipantLocationListener on_data_available: narrow failed.");;
        System.exit(1);
      }

      ParticipantLocationBuiltinTopicDataHolder loc =
        new ParticipantLocationBuiltinTopicDataHolder(new ParticipantLocationBuiltinTopicData());
      SampleInfoHolder sih = new SampleInfoHolder(new SampleInfo(0, 0, 0,
        new DDS.Time_t(), 0, 0, 0, 0, 0, 0, 0, false, 0));
      int status = bitDataReader.take_next_sample(loc, sih);

      if (status == RETCODE_OK.value) {

        System.out.println("local = "
                                + loc.value.local_addr);
        System.out.println("relay = "
                                + loc.value.relay_addr);
        System.out.println("ice =  "
                                + loc.value.ice_addr);
      }
      else {
        System.err.println("Error: " + status);
      }
    */
    }

    public void on_requested_deadline_missed(DDS.DataReader reader, DDS.RequestedDeadlineMissedStatus status) {
        System.err.println("ParticipantLocationListener.on_requested_deadline_missed");
    }

    public void on_requested_incompatible_qos(DDS.DataReader reader, DDS.RequestedIncompatibleQosStatus status) {
        System.err.println("ParticipantLocationListener.on_requested_incompatible_qos");
    }

    public void on_sample_rejected(DDS.DataReader reader, DDS.SampleRejectedStatus status) {
        System.err.println("ParticipantLocationListener.on_sample_rejected");
    }

    public void on_liveliness_changed(DDS.DataReader reader, DDS.LivelinessChangedStatus status) {
        System.err.println("ParticipantLocationListener.on_liveliness_changed");
    }

    public void on_subscription_matched(DDS.DataReader reader, DDS.SubscriptionMatchedStatus status) {
        System.err.println("ParticipantLocationListener.on_subscription_matched");
    }

    public void on_sample_lost(DDS.DataReader reader, DDS.SampleLostStatus status) {
        System.err.println("ParticipantLocationListener.on_sample_lost");
    }

}
