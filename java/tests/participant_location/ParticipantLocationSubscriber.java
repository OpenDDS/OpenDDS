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

public class ParticipantLocationSubscriber {
  private static final int N_MSGS = 20;
  private static final int DOMAIN_ID = 42;

  private static DomainParticipantFactory dpf;
  private static DomainParticipant participant;

  private static boolean noIce = false;

  public static void main(String[] args) throws Exception {
    System.out.println("Start Subscriber");
    dpf = TheParticipantFactory.WithArgs(new StringSeqHolder(args));

    for (String s: args) {
      if (s.equals("-n")) {
        noIce = true;
      }
    }

    participant = dpf.create_participant(DOMAIN_ID, PARTICIPANT_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);

    MessageTypeSupport typeSupport = new MessageTypeSupportImpl();
    if (typeSupport.register_type(participant, "Messenger::Message") != RETCODE_OK.value) {
      throw new IllegalStateException("Unable to register type!");
    }

    Topic top = participant.create_topic("Movie Discussion List", typeSupport.get_type_name(), TOPIC_QOS_DEFAULT.get(),
        null, DEFAULT_STATUS_MASK.value);

    Subscriber builtinSubscriber = participant.get_builtin_subscriber();
    if (builtinSubscriber == null) {
      System.err.println("ERROR: could not get built-in subscriber");
      return;
    }

    DataReader dr = builtinSubscriber.lookup_datareader(BuiltinTopicUtils.BUILT_IN_PARTICIPANT_LOCATION_TOPIC);
    if (dr == null) {
      System.err.println("ERROR: could not lookup datareader");
      return;
    }

    ParticipantLocationListener locationListener = new ParticipantLocationListener("Subscriber");
    assert (locationListener != null);

    int ret = dr.set_listener(locationListener, OpenDDS.DCPS.DEFAULT_STATUS_MASK.value);
    assert (ret == DDS.RETCODE_OK.value);

    Subscriber sub = participant.create_subscriber(SUBSCRIBER_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
    if (sub == null) {
      System.err.println("ERROR: Subscriber creation failed");
      return;
    }

    DataReader subReader = sub.create_datareader(top, DATAREADER_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
    if (subReader == null) {
      System.err.println("ERROR: Subscriber DataReader creation failed");
      return;
    }

    WaitSet ws = new WaitSet();

    Message msg = new Message();
    MessageDataReader mdr = MessageDataReaderHelper.narrow(subReader);
    ReadCondition rc = subReader.create_readcondition(ANY_SAMPLE_STATE.value, ANY_VIEW_STATE.value,
        ALIVE_INSTANCE_STATE.value);
    ws.attach_condition(rc);

    Duration_t timeout = new Duration_t(DURATION_INFINITE_SEC.value,
                                            DURATION_INFINITE_NSEC.value);

    while (true) {
      ConditionSeqHolder cond = new ConditionSeqHolder(new Condition[] {});
      if (ws.wait(cond, timeout) != RETCODE_OK.value) {
        System.err.println("ERROR: DataReader wait() failed.");
        return;
      }
      MessageHolder mholder = new MessageHolder(msg);
      SampleInfoHolder infoholder = new SampleInfoHolder(new SampleInfo());
      infoholder.value.source_timestamp = new Time_t();
      if (mdr.take_next_sample(mholder, infoholder) != RETCODE_OK.value) {
        System.err.println("ERROR: take_next_sample");
        return;
      }
      else {
        System.out.println("Got msg " + msg.count + ". '" + msg.text + "'");
      }

      if (msg.count == N_MSGS - 1) {
        System.out.println("Got last expected message.");
        break;
      }
    }
    ws.detach_condition(rc);
    dr.delete_readcondition(rc);

    System.out.println("Stop Subscriber");

    Thread.sleep(5000);
    boolean success = locationListener.check(noIce);

    // cleanup
    participant.delete_contained_entities();
    dpf.delete_participant(participant);
    TheServiceParticipant.shutdown();

    if (!success) {
      System.exit(1);
    }

  }

}
