/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Argv_Type_Converter.h>
#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsCoreTypeSupportImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "DataReaderListener.h"
#include "MessengerTypeSupportImpl.h"

#include "tests/Utils/StatusMatching.h"
#include <tests/Utils/DistributedConditionSet.h>

#include <iostream>

int testcase = strength;

DDS::Duration_t deadline = {DDS::DURATION_INFINITE_SEC,
                            DDS::DURATION_INFINITE_NSEC};
DDS::Duration_t liveliness = {DDS::DURATION_INFINITE_SEC,
                              DDS::DURATION_INFINITE_NSEC};
int
parse_args(int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts(argc, argv, ACE_TEXT("d:l:t:"));

  int c;
  while ((c = get_opts()) != -1) {
    switch (c) {
    case 'd':
      deadline.sec = ACE_OS::atoi (get_opts.opt_arg());
      deadline.nanosec = 0;
      break;
    case 'l':
      liveliness.sec = ACE_OS::atoi (get_opts.opt_arg());
      liveliness.nanosec = 0;
      break;
    case 't':
      testcase = ACE_OS::atoi (get_opts.opt_arg());
      break;
    case '?':
    default:
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("usage: %C -d <deadline> -l <liveliness> ")
                        ACE_TEXT("-t <testcase>\n"), argv[0]),
                       -1);
    }
  }

  return 0;
}

class BitDataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  BitDataReaderListenerImpl(DistributedConditionSet_rch dcs)
    : dcs_(dcs)
  {}

  void on_requested_deadline_missed(DDS::DataReader_ptr,
                                    const DDS::RequestedDeadlineMissedStatus&)
  {}

  void on_requested_incompatible_qos(DDS::DataReader_ptr,
                                     const DDS::RequestedIncompatibleQosStatus&)
  {}

  void on_liveliness_changed(DDS::DataReader_ptr,
                             const DDS::LivelinessChangedStatus&)
  {}

  void on_subscription_matched(DDS::DataReader_ptr,
                               const DDS::SubscriptionMatchedStatus&)
  {}

  void on_sample_rejected(DDS::DataReader_ptr,
                          const DDS::SampleRejectedStatus&)
  {}

  void on_data_available(DDS::DataReader_ptr reader)
  {
    DDS::PublicationBuiltinTopicDataDataReader_var pub_bit =
      DDS::PublicationBuiltinTopicDataDataReader::_narrow(reader);

    DDS::PublicationBuiltinTopicDataSeq data;
    DDS::SampleInfoSeq info;
    pub_bit->read(data, info, DDS::LENGTH_UNLIMITED,
                  DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
    for (unsigned int idx = 0; idx != data.length(); ++idx) {
      if (info[idx].valid_data) {
        dcs_->post("subscriber", "strength change " + OpenDDS::DCPS::to_dds_string(data[idx].ownership_strength.value));
      }
    }
  }

  void on_sample_lost(DDS::DataReader_ptr,
                      const DDS::SampleLostStatus&)
  {}

  DistributedConditionSet_rch dcs_;
};

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try {
    DistributedConditionSet_rch dcs =
      OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);
    DDS::DomainParticipant_var participant;

    bool result1, result2;

    int error;
    if ((error = parse_args(argc, argv)) != 0) {
      ACE_ERROR((LM_ERROR, "(%P|%t) Parsing error, returning %d\n", error));
      return error;
    }

    {  // Scope of entities
      // Create DomainParticipant
      participant =
        dpf->create_participant(111,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(participant.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: create_participant() failed!\n")), -1);
      }

      DDS::DataReaderListener_var bit_listener(new BitDataReaderListenerImpl(dcs));
      DDS::Subscriber_var bit_sub = participant->get_builtin_subscriber();
      DDS::DataReader_var bit_reader = bit_sub->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC);
      bit_reader->set_listener(bit_listener, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      bit_listener->on_data_available(bit_reader);

      // Register Type (Messenger::Message)
      Messenger::MessageTypeSupport_var ts =
        new Messenger::MessageTypeSupportImpl();

      if (ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: register_type() failed!\n")), -1);
      }

      // Create Topic (Movie Discussion List)
      DDS::Topic_var topic =
        participant->create_topic("Movie Discussion List",
                                  CORBA::String_var(ts->get_type_name()),
                                  TOPIC_QOS_DEFAULT,
                                  DDS::TopicListener::_nil(),
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(topic.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: create_topic() failed!\n")), -1);
      }

      // Create Subscriber
      DDS::Subscriber_var sub =
        participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                       DDS::SubscriberListener::_nil(),
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(sub.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: create_subscriber() failed!\n")), -1);
      }

      // Create DataReader
      DataReaderListenerImpl* listener_svt1 = new DataReaderListenerImpl("DataReader1");
      DataReaderListenerImpl* listener_svt2 = new DataReaderListenerImpl("DataReader2");

      DDS::DataReaderListener_var listener1(listener_svt1);
      DDS::DataReaderListener_var listener2(listener_svt2);

      ::DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos (dr_qos);
      dr_qos.ownership.kind = ::DDS::EXCLUSIVE_OWNERSHIP_QOS;
      dr_qos.deadline.period.sec = deadline.sec;
      dr_qos.deadline.period.nanosec = deadline.nanosec;
      dr_qos.liveliness.lease_duration.sec = liveliness.sec;
      dr_qos.liveliness.lease_duration.nanosec = liveliness.nanosec;
      dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

      DDS::DataReader_var reader1 =
        sub->create_datareader(topic.in(),
                               dr_qos,
                               listener1.in(),
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(reader1.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
      }

      DDS::DataReader_var reader2 =
        sub->create_datareader(topic.in(),
                               dr_qos,
                               listener2.in(),
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(reader2.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
      }

      if (wait_match(reader1, 2, Utils::EQ)) {
        ACE_OS::exit(-1);
      }
      dcs->post("reader1", "ready");
      if (wait_match(reader2, 2, Utils::EQ)) {
        ACE_OS::exit(-1);
      }
      dcs->post("reader2", "ready");

      // Block until Publisher completes
      DDS::StatusCondition_var condition1 = reader1->get_statuscondition();
      DDS::StatusCondition_var condition2 = reader2->get_statuscondition();
      condition1->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);
      condition2->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);

      DDS::WaitSet_var ws = new DDS::WaitSet;
      ws->attach_condition(condition1);
      ws->attach_condition(condition2);

      DDS::Duration_t timeout =
        { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

      DDS::ConditionSeq conditions;
      DDS::SubscriptionMatchedStatus matches1 = { 0, 0, 0, 0, 0 };
      DDS::SubscriptionMatchedStatus matches2 = { 0, 0, 0, 0, 0 };
      while (true) {
        if (reader1->get_subscription_matched_status(matches1) != DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l main()")
                            ACE_TEXT(" ERROR: get_subscription_matched_status() failed!\n")), -1);
        }

        if (reader2->get_subscription_matched_status(matches2) != DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l main()")
                            ACE_TEXT(" ERROR: get_subscription_matched_status() failed!\n")), -1);
        }

        if ((matches1.current_count == 0 && matches1.total_count > 0) ||
            (matches2.current_count == 0 && matches2.total_count > 0)) {
          break;
        }
        DDS::ReturnCode_t wait_status = ws->wait(conditions, timeout);
        if (wait_status != DDS::RETCODE_OK) {
          std::cerr << "ERROR: Subscriber failed during waiting on wait set with return code "
                    << wait_status << std::endl;
        }
      }

      ws->detach_condition(condition1);
      ws->detach_condition(condition2);

      result1 = listener_svt1->verify_result();
      result2 = listener_svt2->verify_result();
    }  // Scope of entities

    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());

    TheServiceParticipant->shutdown();


    if (result1 == false || result2 == false) {
      ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: failed to verify message!\n")), -2);
    }

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  }

  return 0;
}
