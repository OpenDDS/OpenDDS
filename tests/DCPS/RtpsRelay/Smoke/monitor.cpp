/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/rtpsrelaylib/RelayTypeSupportImpl.h>
#include <dds/rtpsrelaylib/Utility.h>

#include <dds/DCPS/JsonValueWriter.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#  ifdef OPENDDS_SECURITY
#    include <dds/DCPS/security/BuiltInPlugins.h>
#  endif
#endif

void append(DDS::PropertySeq& props, const char* name, const std::string& value, bool propagate = false)
{
  const DDS::Property_t prop = {name, value.c_str(), propagate};
  const unsigned int len = props.length();
  props.length(len + 1);
  try {
    props[len] = prop;
  } catch (const CORBA::BAD_PARAM& /*ex*/) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Exception caught when appending parameter\n"));
  }
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  DDS::DomainParticipantFactory_var factory = TheParticipantFactoryWithArgs(argc, argv);
  if (!factory) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Failed to initialize participant factory\n")));
    return EXIT_FAILURE;
  }

  DDS::DomainId_t relay_domain = 0;

  // Set up the relay participant.
  DDS::DomainParticipantQos participant_qos;
  factory->get_default_participant_qos(participant_qos);
  DDS::PropertySeq& relay_properties = participant_qos.property.value;
  append(relay_properties, OpenDDS::RTPS::RTPS_REFLECT_HEARTBEAT_COUNT, "true");

  DDS::DomainParticipant_var relay_participant = factory->create_participant(relay_domain, participant_qos, nullptr,
                                                                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!relay_participant) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Failed to create relay participant\n")));
    return EXIT_FAILURE;
  }

  // Set up relay topics.
  RtpsRelay::RelayParticipantStatusTypeSupport_var relay_participant_status_ts = new RtpsRelay::RelayParticipantStatusTypeSupportImpl;
  if (relay_participant_status_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to register RelayParticipantStatus type\n")));
    return EXIT_FAILURE;
  }
  CORBA::String_var relay_participant_status_type_name = relay_participant_status_ts->get_type_name();

  DDS::Topic_var relay_participant_status_topic =
    relay_participant->create_topic(RtpsRelay::RELAY_PARTICIPANT_STATUS_TOPIC_NAME.c_str(),
                                    relay_participant_status_type_name,
                                    TOPIC_QOS_DEFAULT, nullptr,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_participant_status_topic) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Relay Participant Status topic\n")));
    return EXIT_FAILURE;
  }

  // Setup relay subscriber.
  DDS::SubscriberQos subscriber_qos;
  relay_participant->get_default_subscriber_qos(subscriber_qos);
  subscriber_qos.partition.name.length(1);
  subscriber_qos.partition.name[0] = "*"; // Subscribe to all partitions.

  DDS::Subscriber_var relay_subscriber = relay_participant->create_subscriber(subscriber_qos, nullptr,
                                                                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_subscriber) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Relay subscriber\n")));
    return EXIT_FAILURE;
  }

  // Setup relay reader/writer qos.
  DDS::DataReaderQos reader_qos;
  relay_subscriber->get_default_datareader_qos(reader_qos);

  reader_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  reader_qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
  reader_qos.history.depth = 1;

  DDS::DataReader_var relay_participant_status_dr =
    relay_subscriber->create_datareader(relay_participant_status_topic.in(),
                                        reader_qos,
                                        0,
                                        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_participant_status_dr) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: create_datareader failed\n"),
                     -1);
  }

  RtpsRelay::RelayParticipantStatusDataReader_var relay_participant_status_reader = RtpsRelay::RelayParticipantStatusDataReader::_narrow(relay_participant_status_dr.in());

  if (!relay_participant_status_reader) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: _narrow failed\n"),
                     -1);
  }

  DDS::ReadCondition_var rc = relay_participant_status_reader->create_readcondition(DDS::ANY_SAMPLE_STATE,
                                                                                    DDS::ANY_VIEW_STATE,
                                                                                    DDS::ANY_INSTANCE_STATE);
  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(rc);
  while (true) {
    DDS::ConditionSeq active;
    const DDS::Duration_t max_wait = {DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
    ws->wait(active, max_wait);
    RtpsRelay::RelayParticipantStatusSeq data;
    DDS::SampleInfoSeq info;
    DDS::ReturnCode_t ret;
    while ((ret = relay_participant_status_reader->take_w_condition(data, info, DDS::LENGTH_UNLIMITED, rc)) == DDS::RETCODE_OK) {
#if OPENDDS_HAS_JSON_VALUE_WRITER
      for (CORBA::ULong idx = 0; idx != data.length(); ++idx) {
        std::cout << "MONITOR " << RtpsRelay::RELAY_PARTICIPANT_STATUS_TOPIC_NAME.c_str() << ' ' << OpenDDS::DCPS::to_json(info[idx]);
        if (info[idx].valid_data) {
          std::cout << ' ' << OpenDDS::DCPS::to_json(data[idx]);
        }
        std::cout << std::endl;
      }
#endif
    }
  }

  return EXIT_SUCCESS;
}
