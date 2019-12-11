/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

import DDS.*;
import OpenDDS.DCPS.*;

import org.omg.CORBA.StringSeqHolder;

import Messenger.*;


public class ParticipantLocationTest {
  private static final int DOMAIN_ID = 42;

  private static DomainParticipantFactory dpf;
  private static DomainParticipant participant;

  protected static void testParticipantLocationBIT(String[] args) throws Exception {
    System.out.println("test ParticipantLocation");

    dpf = TheParticipantFactory.WithArgs(new StringSeqHolder(args));

    participant = dpf.create_participant(DOMAIN_ID, PARTICIPANT_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);

    MessageTypeSupport typeSupport = new MessageTypeSupportImpl();
    if (typeSupport.register_type(participant, "Messenger::Message") != RETCODE_OK.value) {
      throw new IllegalStateException("Unable to register type!");
    }

    Topic top = participant.create_topic("Movie Discussion List",
                                    typeSupport.get_type_name(),
                                    TOPIC_QOS_DEFAULT.get(),
                                    null,
                                    DEFAULT_STATUS_MASK.value);

    Thread.sleep(2500); // Wait for repo to settle

    Subscriber builtinSubscriber = participant.get_builtin_subscriber();
    if (builtinSubscriber  == null) {
      System.err.println("ERROR: could not get built-in subscriber");
      return;
    }

    DataReader dr = builtinSubscriber.lookup_datareader("OpenDDSParticipantLocation");
    if (dr  == null) {
      System.err.println("ERROR: could not lookup datareader");
      return;
    }

    ParticipantLocationListener locationListener = new ParticipantLocationListener();
    assert (locationListener != null);

    int ret = dr.set_listener(locationListener, OpenDDS.DCPS.DEFAULT_STATUS_MASK.value);
    assert (ret == DDS.RETCODE_OK.value);

    //----
    DataReader pdr = builtinSubscriber.lookup_datareader("DCPSParticipant");
    if (pdr  == null) {
      System.err.println("ERROR: could not lookup datareader");
      return;
    }

    ParticipantLocationListener plocationListener = new ParticipantLocationListener();
    assert (plocationListener != null);

    ret = pdr.set_listener(plocationListener, OpenDDS.DCPS.DEFAULT_STATUS_MASK.value);
    assert (ret == DDS.RETCODE_OK.value);

//----

    ParticipantLocationBuiltinTopicDataDataReader reader =
        ParticipantLocationBuiltinTopicDataDataReaderHelper.narrow(dr);

    assert (reader != null);

    ParticipantLocationBuiltinTopicDataSeqHolder data = new ParticipantLocationBuiltinTopicDataSeqHolder(
        new ParticipantLocationBuiltinTopicData[0]);

    SampleInfoSeqHolder info = new SampleInfoSeqHolder(new SampleInfo[0]);

    reader.read(data, info, 1, ANY_SAMPLE_STATE.value, ANY_VIEW_STATE.value, ANY_INSTANCE_STATE.value);

    assert (data.value.length > 0);

    System.out.println("local: " + data.value[0].local_addr);

    Thread.sleep(5000);

    // cleanup
    participant.delete_contained_entities();
    dpf.delete_participant(participant);

  }

  public static void main(String[] args) throws Exception {
    testParticipantLocationBIT(args);
  }

}
