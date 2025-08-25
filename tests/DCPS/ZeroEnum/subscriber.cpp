// -*- C++ -*-
#include "ZeroEnumTypeSupportImpl.h"

#include "tests/Utils/DistributedConditionSet.h"

#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/StaticIncludes.h>
#include <dds/DCPS/WaitSet.h>

#include <dds/DCPS/XTypes/DynamicTypeSupport.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

class TestListener : public DDS::DataReaderListener {
public:
  TestListener(DistributedConditionSet_rch dcs)
    : dcs_(dcs)
  {}

  void on_requested_deadline_missed(DDS::DataReader_ptr,
                                    const DDS::RequestedDeadlineMissedStatus&) override
  {}

  void on_requested_incompatible_qos(DDS::DataReader_ptr,
                                     const DDS::RequestedIncompatibleQosStatus&) override
  {}

  void on_sample_rejected(DDS::DataReader_ptr,
                          const DDS::SampleRejectedStatus&) override
  {}

  void on_liveliness_changed(DDS::DataReader_ptr,
                             const DDS::LivelinessChangedStatus&) override
  {}

  void on_data_available(DDS::DataReader_ptr reader) override
  {
    DDS::DynamicDataReader_var r = DDS::DynamicDataReader::_narrow(reader);

    DDS::DynamicDataSeq datas;
    DDS::SampleInfoSeq infos;

    const DDS::ReturnCode_t ret = r->take(datas, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    if (ret != DDS::RETCODE_OK) {
      return;
    }

    for (CORBA::ULong idx = 0; idx != datas.length(); ++idx) {
      if (infos[idx].valid_data) {
        DDS::DynamicData_ptr dd = datas[idx];
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
    }
  }

  void on_subscription_matched(DDS::DataReader_ptr,
                               const DDS::SubscriptionMatchedStatus&) override
  {
    dcs_->post(ZeroEnum::LISTENER, ZeroEnum::ON_RECORDER_MATCHED);
  }

  void on_sample_lost(DDS::DataReader_ptr,
                      const DDS::SampleLostStatus&) override
  {}

private:
  DistributedConditionSet_rch dcs_;
};

DDS::ReturnCode_t wait_for_writer(DDS::DomainParticipant_var participant,
                                  DDS::DynamicType_var& type)
{

  if (type) {
    return DDS::RETCODE_OK;
  }

  DDS::Subscriber_var bit_subscriber = participant->get_builtin_subscriber();
  DDS::DataReader_var dr = bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC);
  DDS::PublicationBuiltinTopicDataDataReader_var pub_dr = DDS::PublicationBuiltinTopicDataDataReader::_narrow(dr);
  DDS::ReadCondition_var rc = pub_dr->create_readcondition(DDS::NOT_READ_SAMPLE_STATE,
                                                           DDS::ANY_VIEW_STATE,
                                                           DDS::ALIVE_INSTANCE_STATE);

  DDS::WaitSet_var ws = new DDS::WaitSet;
  DDS::ReturnCode_t ret = ws->attach_condition(rc);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: wait_for_writer: attach_condition failed: %C\n", OpenDDS::DCPS::retcode_to_string(ret)));
    return ret;
  }

  DDS::Duration_t timeout = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
  DDS::ConditionSeq conditions;
  while (!type && (ret = ws->wait(conditions, timeout)) == DDS::RETCODE_OK) {
    DDS::PublicationBuiltinTopicDataSeq pub_data;
    DDS::SampleInfoSeq infos;
    ret = pub_dr->read_w_condition(pub_data, infos, DDS::LENGTH_UNLIMITED, rc);
    if (ret == DDS::RETCODE_OK) {
      for (unsigned int idx = 0; !type && idx != pub_data.length(); ++idx) {
        if (std::strcmp(pub_data[idx].topic_name, ZeroEnum::MESSAGE_TOPIC_NAME) == 0) {
          TheServiceParticipant->get_dynamic_type(type, participant, pub_data[idx].key);
        }
      }
    } else {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: wait_for_writer: read_w_condition failed: %C\n", OpenDDS::DCPS::retcode_to_string(ret)));
    }
  }

  ws->detach_condition(rc);
  pub_dr->delete_readcondition(rc);

  return DDS::RETCODE_OK;
}

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

  DDS::DynamicType_var type;
  wait_for_writer(participant, type);

  DDS::TypeSupport_var type_support = new DDS::DynamicTypeSupport(type);
  type_support->register_type(participant, "MyType");

  DDS::Topic_var topic = participant->create_topic(ZeroEnum::MESSAGE_TOPIC_NAME,
                                                   "MyType",
                                                   TOPIC_QOS_DEFAULT,
                                                   0,
                                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::Subscriber_var subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                                                  0,
                                                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::DataReaderQos data_reader_qos = TheServiceParticipant->initial_DataReaderQos();
  data_reader_qos.representation.value.length(1);
  data_reader_qos.representation.value[0] = DDS::XCDR2_DATA_REPRESENTATION;
  data_reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  DDS::DataReaderListener_var listener = new TestListener(distributed_condition_set);

  DDS::DataReader_var reader = subscriber->create_datareader(topic,
                                                             data_reader_qos,
                                                             listener,
                                                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  distributed_condition_set->wait_for(ZeroEnum::LISTENER, ZeroEnum::LISTENER, ZeroEnum::ON_SAMPLE_DATA_RECEIVED);

  participant->delete_contained_entities();
  domain_participant_factory->delete_participant(participant);
  TheServiceParticipant->shutdown();

  return EXIT_SUCCESS;
}
