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

public class ParticipantLocationPublisher {
  private static final int N_MSGS = 20;
  private static final int DOMAIN_ID = 42;

  private static DomainParticipantFactory dpf;
  private static DomainParticipant participant;

  private static boolean noIce = false;

  public static void main (String[] args) throws Exception {
    System.out.println("Start Publisher");
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

    ParticipantLocationListener locationListener = new ParticipantLocationListener("Publisher");
    assert (locationListener != null);

    int ret = dr.set_listener(locationListener, OpenDDS.DCPS.DEFAULT_STATUS_MASK.value);
    assert (ret == DDS.RETCODE_OK.value);

    Publisher pub = participant.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
    if (pub == null) {
      System.err.println("ERROR: Publisher creation failed");
      return;
    }

    DataWriter dw = pub.create_datawriter(top, DATAWRITER_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);

    if (dw == null) {
      System.err.println("ERROR: DataWriter creation failed");
      return;
    }
    System.out.println("Publisher Created DataWriter");

    StatusCondition sc = dw.get_statuscondition();
    sc.set_enabled_statuses(PUBLICATION_MATCHED_STATUS.value);
    WaitSet ws = new WaitSet();
    ws.attach_condition(sc);
    PublicationMatchedStatusHolder matched = new PublicationMatchedStatusHolder(new PublicationMatchedStatus());
    Duration_t timeout = new Duration_t(DURATION_INFINITE_SEC.value, DURATION_INFINITE_NSEC.value);

    while (true) {
      final int result = dw.get_publication_matched_status(matched);
      if (result != RETCODE_OK.value) {
        System.err.println("ERROR: get_publication_matched_status()" + "failed.");
        return;
      }

      if (matched.value.current_count >= 1) {
        System.out.println("Publisher Matched");
        break;
      }

      ConditionSeqHolder cond = new ConditionSeqHolder(new Condition[] {});
      if (ws.wait(cond, timeout) != RETCODE_OK.value) {
        System.err.println("ERROR: wait() failed.");
        return;
      }
    }

    ws.detach_condition(sc);

    System.out.println("Publisher sending messages");

    MessageDataWriter mdw = MessageDataWriterHelper.narrow(dw);
    Message msg = new Message();
    msg.subject_id = 99;
    int handle = mdw.register_instance(msg);
    msg.from = "OpenDDS-Java";
    msg.subject = "Review";
    msg.text = "Worst. Movie. Ever.";
    msg.count = 0;
    ret = RETCODE_TIMEOUT.value;
    for (; msg.count < N_MSGS; ++msg.count) {
      while ((ret = mdw.write(msg, handle)) == RETCODE_TIMEOUT.value) {
      }
      if (ret != RETCODE_OK.value) {
        System.err.println("ERROR " + msg.count + " write() returned " + ret);
      }
      try {
        Thread.sleep(100);
      } catch (InterruptedException ie) {
      }
    }

    System.out.println("Publisher waiting for acks");

    // Wait for acknowledgements
    Duration_t forever = new Duration_t(DURATION_INFINITE_SEC.value, DURATION_INFINITE_NSEC.value);
    dw.wait_for_acknowledgments(forever);

    System.out.println("Stop Publisher");

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
