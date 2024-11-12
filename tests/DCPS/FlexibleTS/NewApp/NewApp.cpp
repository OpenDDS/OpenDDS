#include "NewAppTypeSupportImpl.h"

#include <tests/Utils/DistributedConditionSet.h>
#include <tests/Utils/StatusMatching.h>

#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/FlexibleTypeSupport.h>
#include <dds/DCPS/GuidConverter.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>

#include <dds/DCPS/StaticIncludes.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

static const char NEW_TYPE[] = "newType";
static const char OLD_TYPE[] = "oldType";

std::string setup_typesupport(HelloWorld::MessageTypeSupport& base, const DDS::DomainParticipant_var& participant)
{
  using namespace OpenDDS::DCPS;
  using namespace OpenDDS::XTypes;
  RcHandle<FlexibleTypeSupport> flex(make_rch<FlexibleTypeSupport>(ref(base), NEW_TYPE));
  flex->register_type(participant, NEW_TYPE);

  Sequence<DDS::Int32> valuesToRemove;
  valuesToRemove.append(HelloWorld::Intermediate);

  DomainParticipantImpl* participantImpl = dynamic_cast<DomainParticipantImpl*>(participant.in());
  TypeLookupService_rch lookup = participantImpl->get_type_lookup_service();

  TypeSupportImpl& baseImpl = dynamic_cast<TypeSupportImpl&>(base);
  TypeMap minTm, cmpTm;
  TypeIdentifier minOldTi = remove_enumerators(baseImpl.getMinimalTypeIdentifier(),
                                               getMinimalTypeIdentifier<HelloWorld_State_xtag>(),
                                               valuesToRemove, *lookup, minTm);
  TypeIdentifier cmpOldTi = remove_enumerators(baseImpl.getCompleteTypeIdentifier(),
                                               getCompleteTypeIdentifier<HelloWorld_State_xtag>(),
                                               valuesToRemove, *lookup, cmpTm);
  flex->add(OLD_TYPE, minOldTi, minTm, cmpOldTi, cmpTm);
  return NEW_TYPE;
}

void read_message(const HelloWorld::MessageDataReader_var& message_data_reader, bool& done)
{
  HelloWorld::MessageSeq messages;
  DDS::SampleInfoSeq infos;
  message_data_reader->take(messages, infos, DDS::LENGTH_UNLIMITED,
                            DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  for (unsigned int idx = 0; idx != messages.length(); ++idx) {
    if (infos[idx].valid_data) {
      ACE_DEBUG((LM_DEBUG, "received %C\n", messages[idx].value.in()));
      done = true;
      return;
    }
  }
}

void read_pbit(const DDS::DataReader_var& reader)
{
  using namespace OpenDDS::DCPS;
  DDS::ParticipantBuiltinTopicDataDataReader_var bit_reader =
    DDS::ParticipantBuiltinTopicDataDataReader::_narrow(reader);
  DDS::ParticipantBuiltinTopicData bitSample;
  DDS::SampleInfo sampleInfo;
  while (bit_reader->read_next_sample(bitSample, sampleInfo) == DDS::RETCODE_OK) {
    if (sampleInfo.sample_state == DDS::NOT_READ_SAMPLE_STATE && sampleInfo.valid_data) {
      const GUID_t guidRemote = bit_key_to_guid(bitSample.key);
      ACE_DEBUG((LM_DEBUG, "Enable FlexibleTS for %C\n", LogGuid(guidRemote).c_str()));
      DDS::Subscriber_var sub = reader->get_subscriber();
      DDS::DomainParticipant_var participant = sub->get_participant();
      DomainParticipantImpl* participantImpl = dynamic_cast<DomainParticipantImpl*>(participant.in());
      Discovery_rch disco = TheServiceParticipant->get_discovery(HelloWorld::HELLO_WORLD_DOMAIN);
      disco->enable_flexible_types(HelloWorld::HELLO_WORLD_DOMAIN, participantImpl->get_id(), guidRemote, OLD_TYPE);
    }
  }
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  // Coordination across processes.
  DistributedConditionSet_rch distributed_condition_set =
    OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

  DDS::DomainParticipantFactory_var domain_participant_factory = TheParticipantFactoryWithArgs(argc, argv);
  DDS::DomainParticipant_var participant =
    domain_participant_factory->create_participant(HelloWorld::HELLO_WORLD_DOMAIN,
                                                   PARTICIPANT_QOS_DEFAULT, 0, 0);

  DDS::Subscriber_var builtin_sub = participant->get_builtin_subscriber();
  DDS::DataReader_var pbit_reader = builtin_sub->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC);
  DDS::ReadCondition_var pbit_condition = pbit_reader->create_readcondition(DDS::NOT_READ_SAMPLE_STATE,
                                                                            DDS::ANY_VIEW_STATE,
                                                                            DDS::ALIVE_INSTANCE_STATE);

  HelloWorld::MessageTypeSupport_var type_support = new HelloWorld::MessageTypeSupportImpl;
  //CORBA::String_var type_name = type_support->get_type_name();
  //type_support->register_type(participant, "");
  std::string type_name = setup_typesupport(*type_support, participant);

  DDS::Topic_var topic = participant->create_topic(HelloWorld::MESSAGE_TOPIC1_NAME, type_name.c_str(),
                                                   TOPIC_QOS_DEFAULT, 0, 0);

  DDS::Subscriber_var subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, 0);

  DDS::DataReaderQos data_reader_qos;
  subscriber->get_default_datareader_qos(data_reader_qos);
  data_reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  DDS::DataReader_var data_reader = subscriber->create_datareader(topic, data_reader_qos, 0, 0);

  HelloWorld::MessageDataReader_var message_data_reader = HelloWorld::MessageDataReader::_narrow(data_reader);

  DDS::Topic_var topic2 = participant->create_topic(HelloWorld::MESSAGE_TOPIC2_NAME, type_name.c_str(),
                                                    TOPIC_QOS_DEFAULT, 0, 0);

  DDS::Publisher_var publisher = participant->create_publisher(PUBLISHER_QOS_DEFAULT, 0, 0);

  DDS::DataWriter_var data_writer = publisher->create_datawriter(topic2, DATAWRITER_QOS_DEFAULT, 0, 0);

  HelloWorld::MessageDataWriter_var message_data_writer = HelloWorld::MessageDataWriter::_narrow(data_writer);

  Utils::wait_match(data_reader, 1);
  Utils::wait_match(data_writer, 1);
  distributed_condition_set->post(HelloWorld::NEWAPP, HelloWorld::NEWAPP_READY);

  DDS::ReadCondition_var read_condition =
    data_reader->create_readcondition(DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  DDS::WaitSet_var wait_set = new DDS::WaitSet;
  wait_set->attach_condition(read_condition);
  wait_set->attach_condition(pbit_condition);

  bool done = false;
  while (!done) {
    DDS::ConditionSeq conditions;
    const DDS::Duration_t timeout = { 1, 0 };
    const DDS::ReturnCode_t ret = wait_set->wait(conditions, timeout);

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "WaitSet::wait() returned %d\n", ret));
      done = true;
    }
    for (unsigned int i = 0; i < conditions.length(); ++i) {
      if (conditions[i] == read_condition) {
        read_message(message_data_reader, done);
      } else if (conditions[i] == pbit_condition) {
        read_pbit(pbit_reader);
      }
    }
  }

  wait_set->detach_condition(read_condition);
  wait_set->detach_condition(pbit_condition);

  HelloWorld::Message message;
  message.value = "Hello World!";
  message.current = HelloWorld::Complete;
  message_data_writer->write(message, DDS::HANDLE_NIL);

  distributed_condition_set->wait_for(HelloWorld::NEWAPP, HelloWorld::OLDAPP, HelloWorld::OLDAPP_DONE);

  participant->delete_contained_entities();
  domain_participant_factory->delete_participant(participant);
  TheServiceParticipant->shutdown();

  return 0;
}
