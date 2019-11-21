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

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  int status = EXIT_SUCCESS;
  DDS::DomainParticipantFactory_var dpf;
  DDS::DomainParticipant_var participant;

  try {
    std::cout << "Starting publisher" << std::endl;
    dpf = TheParticipantFactoryWithArgs(argc, argv);

    participant = dpf->create_participant(4,
                                          PARTICIPANT_QOS_DEFAULT,
                                          0,
                                          OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!participant) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_participant failed!\n")),
                       EXIT_FAILURE);
    }

    Messenger::MessageTypeSupport_var mts = new Messenger::MessageTypeSupportImpl;

    if (mts->register_type(participant, "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: register_type failed!\n")),
                       EXIT_FAILURE);
    }

    CORBA::String_var type_name = mts->get_type_name();
    DDS::Topic_var topic =
      participant->create_topic("Movie Discussion List",
                                type_name,
                                TOPIC_QOS_DEFAULT,
                                0,
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!topic) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_topic failed!\n")),
                       EXIT_FAILURE);
    }

    DDS::Publisher_var pub =
      participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                    0,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!pub) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_publisher failed!\n")),
                       EXIT_FAILURE);
    }

    DDS::DataWriterQos qos;
    pub->get_default_datawriter_qos(qos);
    qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;

    DDS::DataWriter_var dw =
      pub->create_datawriter(topic,
                             qos,
                             0,
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!dw) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l: main()")
                        ACE_TEXT(" ERROR: create_datawriter failed!\n")),
                       EXIT_FAILURE);
    }

    DDS::StatusCondition_var condition = dw->get_statuscondition();
    condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    DDS::Duration_t timeout =
    { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

    DDS::ConditionSeq conditions;
    DDS::PublicationMatchedStatus matches = {0, 0, 0, 0, 0};

    do {
      if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l: main() ERROR: ")
                          ACE_TEXT("wait failed!\n")),
                         EXIT_FAILURE);
      }

      if (dw->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l: main() ERROR: ")
                          ACE_TEXT("get_publication_matched_status failed!\n")),
                         EXIT_FAILURE);
      }

    } while (matches.current_count < 1);

    ws->detach_condition(condition);

    Messenger::MessageDataWriter_var message_dw =
      Messenger::MessageDataWriter::_narrow(dw);

    if (!message_dw) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%N:%l: main() ERROR: ")
                        ACE_TEXT("_narrow failed!\n")),
                       EXIT_FAILURE);
    }

    Messenger::Message message;
    message.subject_id(99);

    DDS::InstanceHandle_t handle = message_dw->register_instance(message);

    message.from("Comic Book Guy");
    message.subject("Review");
    message.text("Worst. Movie. Ever.");
    message.count(0);

    for (int i = 0; i < 40; ++i) {
      DDS::ReturnCode_t error;
      do {
        error = message_dw->write(message, handle);
      } while (error == DDS::RETCODE_TIMEOUT);

      if (error != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: main()")
                   ACE_TEXT(" ERROR: write returned %d!\n"), error));
      }

      ++message.count();
    }

    std::cout << "Writer wait for ACKS" << std::endl;
    dw->wait_for_acknowledgments(timeout);

    participant->delete_contained_entities();
    dpf->delete_participant(participant);
    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    status = EXIT_FAILURE;
  }

  return status;
}
