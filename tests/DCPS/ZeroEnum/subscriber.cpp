// -*- C++ -*-
#include "ZeroEnumTypeSupportImpl.h"

#include "tests/Utils/DistributedConditionSet.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include "dds/DCPS/StaticIncludes.h"

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

class RecorderListenerImpl : public virtual OpenDDS::DCPS::RecorderListener {
public:
  RecorderListenerImpl(DistributedConditionSet_rch dcs)
    : dcs_(dcs)
  {}

private:
  void on_sample_data_received(OpenDDS::DCPS::Recorder* recorder,
                               const OpenDDS::DCPS::RawDataSample& sample)
  {
    DDS::DynamicData_var dd = recorder->get_dynamic_data(sample);
    DDS::DynamicType_var type = dd->type();
    DDS::DynamicTypeMember_var dtm;
    if (type->get_member_by_name(dtm, "my_enum") != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "ERROR: TEST could get dynamic type member for my_enum"));
      abort();

    }
    DDS::MemberDescriptor_var md;
    if (dtm->get_descriptor(md) != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "ERROR: TEST could get member descriptor for my_enum"));
      abort();
    }
    DDS::DynamicType_ptr enum_type = md->type();
    DDS::TypeDescriptor_var desc;
    if (enum_type->get_descriptor(desc) != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "ERROR: TEST could get type descriptor for my_enum"));
      abort();
    }

    if (desc->kind() != OpenDDS::XTypes::TK_ENUM) {
      ACE_ERROR((LM_ERROR, "ERROR: TEST my_enum is not a TK_ENUM"));
      abort();
    }

    if (desc->extensibility_kind() != DDS::APPENDABLE) {
      ACE_ERROR((LM_ERROR, "ERROR: TEST my_enum does not have the expected extensibility"));
      abort();
    }

    dcs_->post(ZeroEnum::LISTENER, ZeroEnum::ON_SAMPLE_DATA_RECEIVED);
  }

  void on_recorder_matched(OpenDDS::DCPS::Recorder*,
                           const DDS::SubscriptionMatchedStatus&)
  {
    dcs_->post(ZeroEnum::LISTENER, ZeroEnum::ON_RECORDER_MATCHED);
  }

  DistributedConditionSet_rch dcs_;
};

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  DistributedConditionSet_rch distributed_condition_set =
    OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

  DDS::DomainParticipantFactory_var domain_participant_factory = TheParticipantFactoryWithArgs(argc, argv);
  DDS::DomainParticipant_var participant =
    domain_participant_factory->create_participant(ZeroEnum::HELLO_WORLD_DOMAIN,
                                                   PARTICIPANT_QOS_DEFAULT,
                                                   0,
                                                   0);

  ZeroEnum::MessageTypeSupport_var type_support = new ZeroEnum::MessageTypeSupportImpl();
  CORBA::String_var type_name = type_support->get_type_name();

  DDS::Topic_var typeless_topic =
    TheServiceParticipant->create_typeless_topic(participant,
                                                 ZeroEnum::MESSAGE_TOPIC_NAME,
                                                 type_name,
                                                 true,
                                                 TOPIC_QOS_DEFAULT,
                                                 0,
                                                 0);

  DDS::DataReaderQos data_reader_qos = TheServiceParticipant->initial_DataReaderQos();
  data_reader_qos.representation.value.length(1);
  data_reader_qos.representation.value[0] = DDS::XCDR2_DATA_REPRESENTATION;
  data_reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  OpenDDS::DCPS::RcHandle<RecorderListenerImpl> listener = OpenDDS::DCPS::make_rch<RecorderListenerImpl>(distributed_condition_set);

  // Create Recorder
  OpenDDS::DCPS::Recorder_var recorder =
    TheServiceParticipant->create_recorder(participant,
                                           typeless_topic,
                                           SUBSCRIBER_QOS_DEFAULT,
                                           data_reader_qos,
                                           listener);

  distributed_condition_set->wait_for(ZeroEnum::LISTENER, ZeroEnum::LISTENER, ZeroEnum::ON_SAMPLE_DATA_RECEIVED);

  TheServiceParticipant->delete_recorder(recorder);
  participant->delete_contained_entities();
  domain_participant_factory->delete_participant(participant);
  TheServiceParticipant->shutdown();

  return EXIT_SUCCESS;
}
