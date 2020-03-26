/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

import DDS.*;
import OpenDDS.DCPS.*;
import java.util.Map;
import java.util.HashMap;

public class ParticipantLocationListener extends DDS._DataReaderListenerLocalBase {

    private String id;
    private Map<String, Integer> map = new HashMap<String, Integer>();

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

    public ParticipantLocationListener(String id) {
      this.id = id;
    }

    public synchronized void on_data_available(DDS.DataReader reader) {
      ParticipantLocationBuiltinTopicDataDataReader bitDataReader =
        ParticipantLocationBuiltinTopicDataDataReaderHelper.narrow(reader);

      if (bitDataReader == null)
      {
        System.err.println("ParticipantLocationListener on_data_available: narrow failed.");;
        System.exit(1);
      }

      ParticipantLocationBuiltinTopicDataHolder participant =
        new ParticipantLocationBuiltinTopicDataHolder(
          new ParticipantLocationBuiltinTopicData(new byte[16], 0, 0, "", new DDS.Time_t(), "", new DDS.Time_t(), "", new DDS.Time_t()));
      SampleInfoHolder si = new SampleInfoHolder(new SampleInfo(0, 0, 0,
        new DDS.Time_t(), 0, 0, 0, 0, 0, 0, 0, false, 0));

      for (int status = bitDataReader.read_next_sample(participant, si);
        status == DDS.RETCODE_OK.value;
        status = bitDataReader.read_next_sample(participant, si)) {

        System.out.println("== " + id + " Participant Location ==");
        System.out.println(" valid: " + si.value.valid_data);
        String guid = guidFormatter(participant.value.guid);
        System.out.println("  guid: " + guid);

        String locations = "";
        if ((participant.value.location & OpenDDS.DCPS.LOCATION_LOCAL.value) != 0) {
          locations = "LOCAL ";
        }
        if ((participant.value.location & OpenDDS.DCPS.LOCATION_ICE.value) != 0) {
          locations += "ICE ";
        }
        if ((participant.value.location & OpenDDS.DCPS.LOCATION_RELAY.value) != 0) {
          locations += "RELAY ";
        }

        System.out.println("   loc: " + locations);

        String masks = "";
        if ((participant.value.change_mask & OpenDDS.DCPS.LOCATION_LOCAL.value) != 0) {
          masks = "LOCAL ";
        }
        if ((participant.value.change_mask & OpenDDS.DCPS.LOCATION_ICE.value) != 0) {
          masks += "ICE ";
        }
        if ((participant.value.change_mask & OpenDDS.DCPS.LOCATION_RELAY.value) != 0) {
          masks += "RELAY ";
        }

        System.out.println("  mask: " + masks);

        System.out.println(" local: " + participant.value.local_addr);
        System.out.println("      : " + participant.value.local_timestamp);
        System.out.println("   ice: " + participant.value.ice_addr);
        System.out.println("      : " + participant.value.ice_timestamp);
        System.out.println(" relay: " + participant.value.relay_addr);
        System.out.println("      : " + participant.value.relay_timestamp);

        // update locations if SampleInfo is valid
        if (si.value.valid_data) {
            Integer mask = 0;
            if (map.containsKey(guid)) {
                mask = map.get(guid);
            }
            mask |= participant.value.location;
            map.put(guid, mask);
        }
      }
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

    public boolean check(boolean noIce) {
        int expected =
            OpenDDS.DCPS.LOCATION_LOCAL.value |
            (!noIce ? OpenDDS.DCPS.LOCATION_ICE.value : 0) |
            OpenDDS.DCPS.LOCATION_RELAY.value;
        System.out.println(id + " expecting "
                           + ((expected & OpenDDS.DCPS.LOCATION_LOCAL.value) != 0 ? " LOCAL" : "")
                           + ((expected & OpenDDS.DCPS.LOCATION_ICE.value) != 0 ? " ICE" : "")
                           + ((expected & OpenDDS.DCPS.LOCATION_RELAY.value) != 0 ? " RELAY" : "")
                           );

        boolean found = false;
        for (Map.Entry entry : map.entrySet()) {
            String key = (String)entry.getKey();
            Integer mask = (Integer)entry.getValue();
            System.out.println(id + " " + key
                               + ((mask & OpenDDS.DCPS.LOCATION_LOCAL.value) != 0 ? " LOCAL" : "")
                               + ((mask & OpenDDS.DCPS.LOCATION_ICE.value) != 0 ? " ICE" : "")
                               + ((mask & OpenDDS.DCPS.LOCATION_RELAY.value) != 0 ? " RELAY" : "")
                               );
            found = found || mask == expected;
        }

        return found;
    }
}
