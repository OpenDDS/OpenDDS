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

#include <dds/OpenDDSConfigWrapper.h>

#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#  if OPENDDS_CONFIG_SECURITY
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

template <typename T>
void print_samples(typename OpenDDS::DCPS::DDSTraits<T>::DataReaderType::_var_type reader,
                   DDS::ReadCondition_var rc)
{
  DDS::TopicDescription_var topic = reader->get_topicdescription();
  CORBA::String_var name = topic->get_name();

  typename OpenDDS::DCPS::DDSTraits<T>::MessageSequenceType data;
  DDS::SampleInfoSeq info;
  DDS::ReturnCode_t ret;
  while ((ret = reader->take_w_condition(data, info, DDS::LENGTH_UNLIMITED, rc)) == DDS::RETCODE_OK) {
#if OPENDDS_HAS_JSON_VALUE_WRITER
    std::ostringstream oss;
    for (CORBA::ULong idx = 0; idx != data.length(); ++idx) {
      oss << "MONITOR " << name.in() << ' ' << OpenDDS::DCPS::to_json(info[idx]);
      if (info[idx].valid_data) {
        oss << ' ' << OpenDDS::DCPS::to_json(data[idx]);
      }
      oss << std::endl;
    }
    ACE_DEBUG((LM_DEBUG, "%C", oss.str().c_str()));
#endif
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
  RtpsRelay::RelayStatusTypeSupport_var relay_status_ts = new RtpsRelay::RelayStatusTypeSupportImpl;
  if (relay_status_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to register Relay Status type\n")));
    return EXIT_FAILURE;
  }
  CORBA::String_var relay_status_type_name = relay_status_ts->get_type_name();

  DDS::Topic_var relay_status_topic =
    relay_participant->create_topic(RtpsRelay::RELAY_STATUS_TOPIC_NAME.c_str(),
                                    relay_status_type_name,
                                    TOPIC_QOS_DEFAULT, nullptr,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_status_topic) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Relay Status topic\n")));
    return EXIT_FAILURE;
  }

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
  const DDS::Duration_t one_minute = { 60, 0 };

  relay_subscriber->get_default_datareader_qos(reader_qos);

  reader_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  reader_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay = one_minute;
  reader_qos.reader_data_lifecycle.autopurge_disposed_samples_delay = one_minute;
  reader_qos.liveliness.kind = DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
  reader_qos.liveliness.lease_duration = { 30, 0 };

  DDS::DataReader_var relay_status_dr =
    relay_subscriber->create_datareader(relay_status_topic.in(),
                                        reader_qos,
                                        0,
                                        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_status_dr) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: create_datareader failed\n"),
                     -1);
  }

  RtpsRelay::RelayStatusDataReader_var relay_status_reader = RtpsRelay::RelayStatusDataReader::_narrow(relay_status_dr.in());

  if (!relay_status_reader) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: _narrow failed\n"),
                     -1);
  }

  DDS::ReadCondition_var relay_status_rc =
    relay_status_reader->create_readcondition(DDS::ANY_SAMPLE_STATE,
                                              DDS::ANY_VIEW_STATE,
                                              DDS::ANY_INSTANCE_STATE);

  relay_subscriber->get_default_datareader_qos(reader_qos);

  reader_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

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

  DDS::ReadCondition_var relay_participant_status_rc =
    relay_participant_status_reader->create_readcondition(DDS::ANY_SAMPLE_STATE,
                                                          DDS::ANY_VIEW_STATE,
                                                          DDS::ANY_INSTANCE_STATE);

  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(relay_status_rc);
  ws->attach_condition(relay_participant_status_rc);
  while (true) {
    DDS::ConditionSeq active;
    const DDS::Duration_t max_wait = {DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
    ws->wait(active, max_wait);
    for (CORBA::ULong idx = 0; idx != active.length(); ++idx) {
      if (active[idx] == relay_status_rc) {
        print_samples<RtpsRelay::RelayStatus>(relay_status_reader, relay_status_rc);
      } else if (active[idx] == relay_participant_status_rc) {
        print_samples<RtpsRelay::RelayParticipantStatus>(relay_participant_status_reader, relay_participant_status_rc);
      }
    }
  }

  return EXIT_SUCCESS;
}
