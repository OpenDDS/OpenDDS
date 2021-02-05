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

public class ParticipantLocationTest {
  private static final int N_MSGS = 20;
  private static final int DOMAIN_ID = 42;

  private static DomainParticipantFactory dpf;
  private static DomainParticipant pubParticipant;
  private static DomainParticipant subParticipant;

  private static boolean noIce = true;
  private static boolean security = false;
  private static boolean ipv6 = false;

  public static void main (String[] args) throws Exception {

    for (String s: args) {
      if (s.equals("-n")) {
        noIce = true;
      }
      else if (s.equals("-6")) {
        ipv6 = true;
      }
      else if (s.equals("-s")) {
        security = true;
      }
    }

    dpf = TheParticipantFactory.WithArgs(new StringSeqHolder(args));

    DomainParticipantQos pqos = new DomainParticipantQos();
    pqos.entity_factory = new EntityFactoryQosPolicy();
    pqos.user_data = new UserDataQosPolicy(new byte[0]);
    pqos.property = new PropertyQosPolicy(new Property_t[0], new BinaryProperty_t[0]);

    DDS.DomainParticipantQosHolder pubQosHolder = new DDS.DomainParticipantQosHolder(pqos);
    dpf.get_default_participant_qos(pubQosHolder);

    DomainParticipantQos sqos = new DomainParticipantQos();
    sqos.entity_factory = new EntityFactoryQosPolicy();
    sqos.user_data = new UserDataQosPolicy(new byte[0]);
    sqos.property = new PropertyQosPolicy(new Property_t[0], new BinaryProperty_t[0]);

    DDS.DomainParticipantQosHolder subQosHolder = new DDS.DomainParticipantQosHolder(sqos);
    dpf.get_default_participant_qos(subQosHolder);

    DDS.Property_t[] pubProps;
    DDS.Property_t[] subProps;

    if (!security) {
      pubProps = new Property_t[1];
      subProps = new Property_t[1];
    } else {
      pubProps = new Property_t[7];
      subProps = new Property_t[7];
    }

    pubProps[0] = new Property_t("OpenDDS.RtpsRelay.Groups", "Messenger", true);
    subProps[0] = new Property_t("OpenDDS.RtpsRelay.Groups", "Messenger", true);

    if (security) {
      System.out.println("Running test with security");

      String pathToTests;
      try {
        pathToTests = "file:" + System.getenv("DDS_ROOT") + "/tests/";
      } catch (NullPointerException npe) {
        // relative path from java tests
        pathToTests = "file:../../../tests/";
      }

      // certificates
      // common
      String authCaFile = pathToTests + "security/certs/identity/identity_ca_cert.pem";
      String permCaFile = pathToTests + "security/certs/permissions/permissions_ca_cert.pem";
      String governanceFile = "file:./governance_signed.p7s";

      // per participant
      String pubIdCertFile = pathToTests + "security/certs/identity/test_participant_02_cert.pem";
      String pubIdKeyFile = pathToTests +"security/certs/identity/test_participant_02_private_key.pem";
      String pubPermissionsFile = "file:./permissions_publisher_signed.p7s";

      String subIdCertFile = pathToTests + "security/certs/identity/test_participant_03_cert.pem";
      String subIdKeyFile = pathToTests + "security/certs/identity/test_participant_03_private_key.pem";
      String subPermissionsFile = "file:./permissions_subscriber_signed.p7s";

      pubProps[1] = new Property_t("dds.sec.auth.identity_ca", authCaFile, false);
      pubProps[2] = new Property_t("dds.sec.auth.identity_certificate", pubIdCertFile, false);
      pubProps[3] = new Property_t("dds.sec.auth.private_key", pubIdKeyFile, false);
      pubProps[4] = new Property_t("dds.sec.access.permissions_ca", permCaFile, false);
      pubProps[5] = new Property_t("dds.sec.access.governance", governanceFile, false);
      pubProps[6] = new Property_t("dds.sec.access.permissions", pubPermissionsFile, false);

      subProps[1] = new Property_t("dds.sec.auth.identity_ca", authCaFile, false);
      subProps[2] = new Property_t("dds.sec.auth.identity_certificate", subIdCertFile, false);
      subProps[3] = new Property_t("dds.sec.auth.private_key", subIdKeyFile, false);
      subProps[4] = new Property_t("dds.sec.access.permissions_ca", permCaFile, false);
      subProps[5] = new Property_t("dds.sec.access.governance", governanceFile, false);
      subProps[6] = new Property_t("dds.sec.access.permissions", subPermissionsFile, false);
    }

    pubQosHolder.value.property.value = pubProps;
    subQosHolder.value.property.value = subProps;

    // setup publisher
    pubParticipant = dpf.create_participant(DOMAIN_ID, pubQosHolder.value, null, DEFAULT_STATUS_MASK.value);

    MessageTypeSupport pubTypeSupport = new MessageTypeSupportImpl();
    if (pubTypeSupport.register_type(pubParticipant, "Messenger::Message") != RETCODE_OK.value) {
      throw new IllegalStateException("Publisher unable to register type!");
    }

    Topic pubTopic = pubParticipant.create_topic("Movie Discussion List", pubTypeSupport.get_type_name(), TOPIC_QOS_DEFAULT.get(),
        null, DEFAULT_STATUS_MASK.value);

    Subscriber pubBuiltinSubscriber = pubParticipant.get_builtin_subscriber();
    if (pubBuiltinSubscriber == null) {
      System.err.println("ERROR: Publisher could not get built-in subscriber");
      return;
    }

    DataReader pubDr = pubBuiltinSubscriber.lookup_datareader(BuiltinTopicUtils.BUILT_IN_PARTICIPANT_LOCATION_TOPIC);
    if (pubDr == null) {
      System.err.println("ERROR: Publisher could not lookup datareader");
      return;
    }

    ParticipantLocationListener pubLocationListener = new ParticipantLocationListener("Publisher");
    assert (pubLocationListener != null);

    int ret = pubDr.set_listener(pubLocationListener, OpenDDS.DCPS.DEFAULT_STATUS_MASK.value);
    assert (ret == DDS.RETCODE_OK.value);

    Publisher pub = pubParticipant.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
    if (pub == null) {
      System.err.println("ERROR: Publisher creation failed");
      return;
    }

    DataWriter pubDw = pub.create_datawriter(pubTopic, DATAWRITER_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);

    if (pubDw == null) {
      System.err.println("ERROR: DataWriter creation failed");
      return;
    }
    System.out.println("Publisher Created DataWriter");

    StatusCondition sc = pubDw.get_statuscondition();
    sc.set_enabled_statuses(PUBLICATION_MATCHED_STATUS.value);
    WaitSet ws = new WaitSet();
    ws.attach_condition(sc);
    PublicationMatchedStatusHolder matched = new PublicationMatchedStatusHolder(new PublicationMatchedStatus());
    Duration_t timeout = new Duration_t(DURATION_INFINITE_SEC.value, DURATION_INFINITE_NSEC.value);

    // setup subscriber
    subParticipant = dpf.create_participant(DOMAIN_ID, pubQosHolder.value, null, DEFAULT_STATUS_MASK.value);

Thread subThread = new Thread(()-> {
    MessageTypeSupport subTypeSupport = new MessageTypeSupportImpl();
    if (subTypeSupport.register_type(subParticipant, "Messenger::Message") != RETCODE_OK.value) {
      throw new IllegalStateException("Unable to register type!");
    }

    Topic subTopic = subParticipant.create_topic("Movie Discussion List", subTypeSupport.get_type_name(), TOPIC_QOS_DEFAULT.get(),
        null, DEFAULT_STATUS_MASK.value);

    Subscriber subBuiltinSubscriber = subParticipant.get_builtin_subscriber();
    if (subBuiltinSubscriber == null) {
      System.err.println("ERROR: subscriber could not get built-in subscriber");
      return;
    }

    DataReader subDr = subBuiltinSubscriber.lookup_datareader(BuiltinTopicUtils.BUILT_IN_PARTICIPANT_LOCATION_TOPIC);
    if (subDr == null) {
      System.err.println("ERROR: subscriber could not lookup datareader");
      return;
    }

    ParticipantLocationListener subLocationListener = new ParticipantLocationListener("Subscriber");
    assert (subLocationListener != null);

    int sret = subDr.set_listener(subLocationListener, OpenDDS.DCPS.DEFAULT_STATUS_MASK.value);
    assert (sret == DDS.RETCODE_OK.value);

    Subscriber sub = subParticipant.create_subscriber(SUBSCRIBER_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
    if (sub == null) {
      System.err.println("ERROR: Subscriber creation failed");
      return;
    }

    TheTransportRegistry.bind_config("subscriber_config", sub);

    DataReader subReader = sub.create_datareader(subTopic, DATAREADER_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
    if (subReader == null) {
      System.err.println("ERROR: Subscriber DataReader creation failed");
      return;
    }

    WaitSet sws = new WaitSet();

    Message msg = new Message();
    MessageDataReader mdr = MessageDataReaderHelper.narrow(subReader);
    ReadCondition rc = subReader.create_readcondition(ANY_SAMPLE_STATE.value, ANY_VIEW_STATE.value,
        ALIVE_INSTANCE_STATE.value);
    sws.attach_condition(rc);

    Duration_t stimeout = new Duration_t(DURATION_INFINITE_SEC.value,
                                            DURATION_INFINITE_NSEC.value);

    while (true) {
      ConditionSeqHolder cond = new ConditionSeqHolder(new Condition[] {});
      if (sws.wait(cond, stimeout) != RETCODE_OK.value) {
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
    sws.detach_condition(rc);
    subDr.delete_readcondition(rc);

    System.out.println("Stop Subscriber");
  });

    subThread.start();

    //Thread.sleep(5000);
    //boolean success = locationListener.check(noIce);

    // publisher wait for match
    while (true) {
      final int result = pubDw.get_publication_matched_status(matched);
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

    MessageDataWriter mdw = MessageDataWriterHelper.narrow(pubDw);
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
    pubDw.wait_for_acknowledgments(forever);

    System.out.println("Stop Publisher");

    Thread.sleep(5000);
    boolean success = pubLocationListener.check(noIce);

    // cleanup
    subParticipant.delete_contained_entities();
    dpf.delete_participant(subParticipant);

    pubParticipant.delete_contained_entities();
    dpf.delete_participant(pubParticipant);

    TheServiceParticipant.shutdown();

    if (!success) {
      System.exit(1);
    }
  }
}
