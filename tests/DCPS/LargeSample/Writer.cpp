/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_unistd.h>

#include <dds/DdsDcpsPublicationC.h>
#include <dds/DCPS/WaitSet.h>

#include "MessengerTypeSupportC.h"
#include "Writer.h"

#include <sstream>
#include <iomanip>

Writer::Writer(DDS::DataWriter_ptr writer1, DDS::DataWriter_ptr writer2,
               int my_pid)
  : writer1_(DDS::DataWriter::_duplicate(writer1)),
    writer2_(DDS::DataWriter::_duplicate(writer2)),
    timeout_writes_(0),
    my_pid_(my_pid)
{
}

namespace {
  void wait_for_match(DDS::DataWriter_ptr writer, bool match = true)
  {
    DDS::StatusCondition_var condition = writer->get_statuscondition();
    condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    DDS::Duration_t timeout =
      { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

    DDS::ConditionSeq conditions;
    DDS::PublicationMatchedStatus matches = {0, 0, 0, 0, 0};

    while (true) {
      if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: wait_for_match()")
                   ACE_TEXT(" ERROR: get_publication_matched_status failed!\n")));
        ACE_OS::exit(-1);
      }

      if (match ? (matches.current_count < 1) : (matches.current_count > 0)) {
        if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("%N:%l: wait_for_match()")
                     ACE_TEXT(" ERROR: wait failed!\n")));
          ACE_OS::exit(-1);
        }
      } else {
        break;
      }
    }
    ws->detach_condition(condition);
  }
}

void
Writer::extend_sample(Messenger::Message& message)
{
  // Writer ID is 1 or 2
  // Message ID is 0 to 9
  // Lengths will vary from 15k to 165k
  // Over 65K will require multiple fragments for udp/mcast
  message.data.length(calc_sample_length(message.writer_id, message.sample_id));
}

void
Writer::write(bool reliable, int num_messages)
{
  DDS::InstanceHandleSeq handles;

  try {
    // Block until Subscriber is available
    wait_for_match(writer1_);
    wait_for_match(writer2_);
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writers matched\n")));

    // Write samples
    Messenger::MessageDataWriter_var message_dw1
      = Messenger::MessageDataWriter::_narrow(writer1_);
    Messenger::MessageDataWriter_var message_dw2
      = Messenger::MessageDataWriter::_narrow(writer2_);

    if (CORBA::is_nil(message_dw1.in())) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: _narrow dw1 failed!\n")));
        ACE_OS::exit(-1);
    }

    if (CORBA::is_nil(message_dw2.in())) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: _narrow dw2 failed!\n")));
        ACE_OS::exit(-1);
    }

    std::ostringstream pid;
    pid << std::setw(5) << my_pid_;

    Messenger::Message message1;
    message1.writer_id  = 1;
    message1.from       = "Comic Book Guy 1";
    message1.process_id = pid.str().c_str();
    message1.text       = "Worst. Movie. Ever.";
    message1.sample_id  = 0;

    Messenger::Message message2 = message1;
    message2.writer_id  = 2;
    message2.from       = "Comic Book Guy 2";

    DDS::InstanceHandle_t handle1 = message_dw1->register_instance(message1);
    DDS::InstanceHandle_t handle2 = message_dw2->register_instance(message2);

    for (int i = 0; i < num_messages; i++) {

      // Because the reader does not have infinite buffer space
      if (!reliable) {
        ACE_OS::sleep(ACE_Time_Value(0,100000));
      }

      extend_sample(message1);
      for (CORBA::ULong j = 0; j < message1.data.length(); ++j) {
        message1.data[j] = j % 256;
      }

      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t)%N:%l: Sending Message: process_id = %C ")
                 ACE_TEXT("writer_id = %d ")
                 ACE_TEXT("sample_id = %d ")
                 ACE_TEXT("extra data length = %d\n"),
                 message1.process_id.in(),
                 message1.writer_id,
                 message1.sample_id,
                 message1.data.length()));
      DDS::ReturnCode_t error;
      do {
        error = message_dw1->write(message1, handle1);

        if (error != DDS::RETCODE_OK) {
          if (error == DDS::RETCODE_TIMEOUT) {
            timeout_writes_++;
          } else {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("%N:%l: svc()")
                       ACE_TEXT(" ERROR: write dw1 returned %d!\n"), error));
          }
        }
      } while (error == DDS::RETCODE_TIMEOUT);

      extend_sample(message2);
      for (CORBA::ULong j = 0; j < message2.data.length(); ++j) {
        message2.data[j] = 255 - (j % 256);
      }

      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t)%N:%l: Sending Message: process_id = %C ")
                 ACE_TEXT("writer_id = %d ")
                 ACE_TEXT("sample_id = %d ")
                 ACE_TEXT("extra data length = %d\n"),
                 message2.process_id.in(),
                 message2.writer_id,
                 message2.sample_id,
                 message2.data.length()));
      do {
        error = message_dw2->write(message2, handle2);
        if (error != DDS::RETCODE_OK) {
          if (error == DDS::RETCODE_TIMEOUT) {
            timeout_writes_++;
          } else {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("%N:%l: svc()")
                       ACE_TEXT(" ERROR: write dw2 returned %d!\n"), error));
          }
        }
      } while (error == DDS::RETCODE_TIMEOUT);

      ++message1.sample_id;
      ++message2.sample_id;
    }

    // Let readers disconnect first, once they either get the data or
    // give up and time-out.  This allows the writer to be alive while
    // processing requests for retransmission from the readers.
    wait_for_match(writer1_, false);
    wait_for_match(writer2_, false);

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in svc():");
  }
}

int
Writer::get_timeout_writes() const
{
  return timeout_writes_.value();
}
