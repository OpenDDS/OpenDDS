/*
 * $Id$
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

const int num_instances_per_writer = 1;
const int num_messages = 10;

Writer::Writer(DDS::DataWriter_ptr writer1, DDS::DataWriter_ptr writer2)
  : writer1_(DDS::DataWriter::_duplicate(writer1)),
    writer2_(DDS::DataWriter::_duplicate(writer2)),
    finished_instances_(0),
    timeout_writes_(0)
{
}

void
Writer::start()
{
  // Lanuch num_instances_per_writer threads. Each thread writes one
  // instance which uses the thread id as the key value.
  if (activate(THR_NEW_LWP | THR_JOINABLE, num_instances_per_writer) == -1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("%N:%l: start()")
               ACE_TEXT(" activate failed!\n")));
    ACE_OS::exit(-1);
  }
}

void
Writer::end()
{
  wait();
}

namespace {
  void wait_for_match(DDS::DataWriter_ptr writer)
  {
    DDS::StatusCondition_var condition = writer->get_statuscondition();
    condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);

    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);

    DDS::Duration_t timeout =
      { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

    DDS::ConditionSeq conditions;
    DDS::PublicationMatchedStatus matches = {0, 0, 0, 0, 0};

    do {
      if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: wait_for_match()")
                   ACE_TEXT(" ERROR: wait failed!\n")));
        ACE_OS::exit(-1);
      }

      if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: wait_for_match()")
                   ACE_TEXT(" ERROR: get_publication_matched_status failed!\n")));
        ACE_OS::exit(-1);
      }

    } while (matches.current_count < 1);

    ws->detach_condition(condition);
  }
}

int
Writer::svc()
{
  DDS::InstanceHandleSeq handles;

  try {
    // Block until Subscriber is available
    wait_for_match(writer1_);
    wait_for_match(writer2_);

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
    pid << ACE_OS::getpid();

    Messenger::Message message1;
    message1.subject_id = 1;
    message1.from       = "Comic Book Guy 1";
    message1.subject    = pid.str().c_str();
    message1.text       = "Worst. Movie. Ever.";
    message1.count      = 0;

    Messenger::Message message2 = message1;
    message2.subject_id = 2;
    message2.from       = "Comic Book Guy 2";

    message1.data.length(66 * 1000); // requires 2 fragments for udp/mcast
    message2.data.length(66 * 1000); // requires 2 fragments for udp/mcast
    for (CORBA::ULong j = 0; j < message1.data.length(); ++j) {
      message1.data[j] = j % 256;
      message2.data[j] = 255 - (j % 256);
    }

    DDS::InstanceHandle_t handle1 = message_dw1->register_instance(message1);
    DDS::InstanceHandle_t handle2 = message_dw2->register_instance(message2);

    for (int i = 0; i < num_messages; i++) {
      ACE_OS::sleep(1);

      DDS::ReturnCode_t error = message_dw1->write(message1, handle1);

      if (error != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: write dw1 returned %d!\n"), error));

        if (error == DDS::RETCODE_TIMEOUT) {
          timeout_writes_++;
        }
      }

      ACE_OS::sleep(ACE_Time_Value(1));

      error = message_dw2->write(message2, handle2);

      if (error != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: svc()")
                   ACE_TEXT(" ERROR: write dw2 returned %d!\n"), error));

        if (error == DDS::RETCODE_TIMEOUT) {
          timeout_writes_++;
        }
      }

      ++message1.count;
      ++message2.count;
    }

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in svc():");
  }

  finished_instances_ ++;

  return 0;
}

bool
Writer::is_finished() const
{
  return finished_instances_ == num_instances_per_writer;
}

int
Writer::get_timeout_writes() const
{
  return timeout_writes_.value();
}
