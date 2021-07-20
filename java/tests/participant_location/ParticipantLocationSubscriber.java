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
import java.util.concurrent.Callable;
import java.util.concurrent.CountDownLatch;

class ParticipantLocationSubscriber implements Callable<Boolean> {
  private final DomainParticipant participant;
  private final int numMsgs;
  private final boolean noIce;
  private final boolean noRelay;

  ParticipantLocationSubscriber(DomainParticipant part, int num, boolean noIce, boolean noRelay) {
    participant = part;
    numMsgs = num;
    this.noIce = noIce;
    this.noRelay = noRelay;
  }

  @Override
  public Boolean call() throws Exception {
    MessageTypeSupport typeSupport = new MessageTypeSupportImpl();
    if (typeSupport.register_type(participant, "Messenger::Message") != RETCODE_OK.value) {
      throw new IllegalStateException("Unable to register type!");
    }

    Topic topic = participant.create_topic("Movie Discussion List", typeSupport.get_type_name(),
                                           TOPIC_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);

    Subscriber builtinSubscriber = participant.get_builtin_subscriber();
    if (builtinSubscriber == null) {
      System.err.println("ERROR: subscriber could not get built-in subscriber");
      return false;
    }

    DataReader dr = builtinSubscriber.lookup_datareader(BuiltinTopicUtils.BUILT_IN_PARTICIPANT_LOCATION_TOPIC);
    if (dr == null) {
      System.err.println("ERROR: subscriber could not lookup datareader");
      return false;
    }

    CountDownLatch latch = new CountDownLatch(1);
    ParticipantLocationListener locationListener = new ParticipantLocationListener("Subscriber", noIce, noRelay, latch);
    assert (locationListener != null);

    int ret = dr.set_listener(locationListener, OpenDDS.DCPS.DEFAULT_STATUS_MASK.value);
    assert (ret == DDS.RETCODE_OK.value);

    Subscriber sub = participant.create_subscriber(SUBSCRIBER_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
    if (sub == null) {
      System.err.println("ERROR: Subscriber creation failed");
      return false;
    }

    // set transport config for subscriber participant
    TheTransportRegistry.bind_config("subscriber_config", sub);

    DataReader reader = sub.create_datareader(topic, DATAREADER_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
    if (reader == null) {
      System.err.println("ERROR: Subscriber DataReader creation failed");
      return false;
    }

    WaitSet ws = new WaitSet();

    Message msg = new Message();
    MessageDataReader mdr = MessageDataReaderHelper.narrow(reader);
    ReadCondition rc = reader.create_readcondition(ANY_SAMPLE_STATE.value, ANY_VIEW_STATE.value,
                                                   ALIVE_INSTANCE_STATE.value);
    ws.attach_condition(rc);

    Duration_t stimeout = new Duration_t(DURATION_INFINITE_SEC.value,
                                         DURATION_INFINITE_NSEC.value);

    while (true) {
      ConditionSeqHolder cond = new ConditionSeqHolder(new Condition[] {});
      if (ws.wait(cond, stimeout) != RETCODE_OK.value) {
        System.err.println("ERROR: Subscriber DataReader wait() failed.");
        return false;
      }
      MessageHolder mholder = new MessageHolder(msg);
      SampleInfoHolder infoholder = new SampleInfoHolder(new SampleInfo());
      infoholder.value.source_timestamp = new Time_t();
      if (mdr.take_next_sample(mholder, infoholder) != RETCODE_OK.value) {
        System.err.println("ERROR: Subscriber take_next_sample");
        return false;
      }
      else {
        System.out.println("Got msg " + msg.count + ". '" + msg.text + "'");
      }
       if (msg.count == numMsgs - 1) {
        System.out.println("Got last expected message.");
        break;
      }
    }
    ws.detach_condition(rc);
    dr.delete_readcondition(rc);

    // wait for location messages
    System.out.println("Subscriber waiting for location messages.");
    latch.await();

    System.out.println("Stop Subscriber");

    // check location received status
    return locationListener.check();
  }

}
