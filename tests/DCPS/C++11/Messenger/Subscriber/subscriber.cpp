/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "../Idl/MessengerTypeSupportImpl.h"

#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/StaticIncludes.h"
#include "dds/DCPS/WaitSet.h"

#include "ace/Log_Msg.h"

#include <iostream>
#include <cstdlib>

namespace {
  const auto num_messages = 40;
}

struct DataReaderListenerImpl
  : virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {

  bool valid_ = true;
  std::set<int> counts_;

  void on_requested_deadline_missed(DDS::DataReader*,
                                    const DDS::RequestedDeadlineMissedStatus&)
  {}

  void on_requested_incompatible_qos(DDS::DataReader*,
                                     const DDS::RequestedIncompatibleQosStatus&)
  {}

  void on_liveliness_changed(DDS::DataReader*,
                             const DDS::LivelinessChangedStatus&)
  {}

  void on_subscription_matched(DDS::DataReader*,
                               const DDS::SubscriptionMatchedStatus&)
  {}

  void on_sample_rejected(DDS::DataReader*,
                          const DDS::SampleRejectedStatus&)
  {}

  void on_sample_lost(DDS::DataReader*,
                      const DDS::SampleLostStatus&)
  {}

  void on_data_available(DDS::DataReader* reader)
  {
    try {
      Messenger::MessageDataReader_var message_dr =
        Messenger::MessageDataReader::_narrow(reader);

      if (!message_dr) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: on_data_available()")
                   ACE_TEXT(" ERROR: _narrow failed!\n")));
        ACE_OS::exit(EXIT_FAILURE);
      }

      Messenger::Message message;
      DDS::SampleInfo si;
      DDS::ReturnCode_t status = message_dr->take_next_sample(message, si);

      if (status == DDS::RETCODE_OK) {
        std::cout << "SampleInfo.sample_rank = " << si.sample_rank << std::endl;
        std::cout << "SampleInfo.instance_state = " << si.instance_state << std::endl;

        if (si.valid_data) {
          if (!counts_.insert(message.count()).second) {
            std::cout << "ERROR: Repeat ";
            valid_ = false;
          }

          std::cout << "Message: subject    = " << message.subject() << std::endl
            << "         subject_id = " << message.subject_id() << std::endl
            << "         from       = " << message.from() << std::endl
            << "         count      = " << message.count() << std::endl
            << "         text       = " << message.text() << std::endl;

        } else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is disposed\n")));

        } else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is unregistered\n")));

        } else {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("%N:%l: on_data_available()")
                     ACE_TEXT(" ERROR: unknown instance state: %d\n"),
                     si.instance_state));
          valid_ = false;
        }

      } else {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: on_data_available()")
                   ACE_TEXT(" ERROR: unexpected status: %d\n"),
                   status));
        valid_ = false;
      }

    } catch (const CORBA::Exception& e) {
      e._tao_print_exception("Exception caught in on_data_available():");
      ACE_OS::exit(EXIT_FAILURE);
    }
  }

  bool is_valid() const
  {
    int expected = 0;
    auto count = counts_.begin();
    bool valid_count = true;
    while (count != counts_.end() && expected < num_messages) {
      if (expected != *count) {
        if (expected < *count) {
          const bool multi = (expected + 1 < *count);
          std::cout << "ERROR: missing message" << (multi ? "s" : "")
            << " with count=" << expected;
          if (multi) {
            std::cout << " to count=" << (*count - 1);
          }
          std::cout << std::endl;
          expected = *count;
          // don't increment count;
          valid_count = false;
          continue;
        } else {
          bool multi = false;
          while (++count != counts_.end() && *count < expected) {
            multi = true;
          }
          std::cout << "ERROR: received message" << (multi ? "s" : "")
            << " with a negative count" << std::endl;
          valid_count = false;
          continue;
        }
      }

      ++expected;
      ++count;
    }

    if (count != counts_.end()) {
      std::cout << "ERROR: received messages with count higher than expected values" << std::endl;
      valid_count = false;

    } else if ((int)counts_.size() < num_messages) {
      std::cout << "ERROR: received " << counts_.size() << " messages, but expected " << num_messages << std::endl;
      valid_count = false;
    }

    return valid_ && valid_count;
  }
};

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  int status = EXIT_SUCCESS;
  try {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    DDS::DomainParticipant_var participant =
      dpf->create_participant(4,
                              PARTICIPANT_QOS_DEFAULT,
                              0,
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!participant) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_participant() failed!\n")),
                       EXIT_FAILURE);
    }

    Messenger::MessageTypeSupport_var ts = new Messenger::MessageTypeSupportImpl;

    if (ts->register_type(participant, "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type() failed!\n")),
                       EXIT_FAILURE);
    }

    CORBA::String_var type_name = ts->get_type_name();
    DDS::Topic_var topic =
      participant->create_topic("Movie Discussion List",
                                type_name,
                                TOPIC_QOS_DEFAULT,
                                0,
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!topic) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_topic() failed!\n")),
                       EXIT_FAILURE);
    }

    DDS::Subscriber_var sub =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                     0,
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!sub) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_subscriber() failed!\n")),
                       EXIT_FAILURE);
    }

    DataReaderListenerImpl* const listener_servant = new DataReaderListenerImpl;
    DDS::DataReaderListener_var listener(listener_servant);

    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    DDS::DataReader_var reader =
      sub->create_datareader(topic,
                             dr_qos,
                             listener,
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!reader) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")),
                       EXIT_FAILURE);
    }

    DDS::StatusCondition_var condition = reader->get_statuscondition();
    condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    DDS::Duration_t timeout =
      { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

    DDS::ConditionSeq conditions;
    DDS::SubscriptionMatchedStatus matches = { 0, 0, 0, 0, 0 };

    while (true) {
      if (reader->get_subscription_matched_status(matches) != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: get_subscription_matched_status() failed!\n")),
                         EXIT_FAILURE);
      }
      if (matches.current_count == 0 && matches.total_count > 0) {
        break;
      }
      if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l main()")
                          ACE_TEXT(" ERROR: wait() failed!\n")),
                         EXIT_FAILURE);
      }
    }

    if (!listener_servant->is_valid()) {
      status = EXIT_FAILURE;
    }

    ws->detach_condition(condition);

    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());
    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    status = EXIT_FAILURE;
  }

  return status;
}
