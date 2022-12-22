/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

import DDS.*;
import OpenDDS.DCPS.*;

public class InternalThreadListener extends DDS._DataReaderListenerLocalBase {
  private String id;

  private String guidFormatter(byte[] guid) {
    StringBuilder g = new StringBuilder();
    for (int ctr = 0; ctr < guid.length; ++ctr) {
      g.append(String.format("%02x", guid[ctr]));
      if ((ctr + 1) %4 == 0 && ctr + 1 < guid.length) {
        g.append(".");
      }
    }
    return g.toString();
  }

  public InternalThreadListener(String id) {
    this.id = id;
  }

  public synchronized void on_data_available(DDS.DataReader reader) {
    InternalThreadBuiltinTopicDataDataReader bitDataReader =
      InternalThreadBuiltinTopicDataDataReaderHelper.narrow(reader);
    if (bitDataReader == null) {
      System.err.println("InternalThreadStatusListener on_data_available: narrow failed.");
      System.exit(1);
    }

    InternalThreadBuiltinTopicDataHolder info =
      new InternalThreadBuiltinTopicDataHolder(
        new InternalThreadBuiltinTopicData("", 0.0));

    SampleInfoHolder si = new SampleInfoHolder(new SampleInfo(0, 0, 0,
      new DDS.Time_t(), 0, 0, 0, 0, 0, 0, 0, false, 0));

    for (int status = bitDataReader.read_next_sample(info, si);
      status == DDS.RETCODE_OK.value;
      status = bitDataReader.read_next_sample(info, si)) {

      System.out.println("== " + id + " Thread Info ==");
      System.out.println("        tid: " + info.value.thread_id);
      System.out.println("utilization: " + info.value.utilization);
      System.out.println("       time: " + si.value.source_timestamp.sec);
    }
  }

  public void on_requested_deadline_missed(DDS.DataReader reader, DDS.RequestedDeadlineMissedStatus status) {
    System.err.println("InternalThreadListener.on_requested_deadline_missed");
  }

  public void on_requested_incompatible_qos(DDS.DataReader reader, DDS.RequestedIncompatibleQosStatus status) {
    System.err.println("InternalThreadListener.on_requested_incompatible_qos");
  }

  public void on_sample_rejected(DDS.DataReader reader, DDS.SampleRejectedStatus status) {
    System.err.println("InternalThreadListener.on_sample_rejected");
  }

  public void on_liveliness_changed(DDS.DataReader reader, DDS.LivelinessChangedStatus status) {
    System.err.println("InternalThreadListener.on_liveliness_changed");
  }

  public void on_subscription_matched(DDS.DataReader reader, DDS.SubscriptionMatchedStatus status) {
    System.err.println("InternalThreadListener.on_subscription_matched");
  }

  public void on_sample_lost(DDS.DataReader reader, DDS.SampleLostStatus status) {
    System.err.println("InternalThreadListener.on_sample_lost");
  }
}
